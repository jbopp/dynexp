// This file is part of DynExp.

#include "stdafx.h"
#include "SignalPlotter.h"

#include "SignalPlotterBackend.h"

namespace DynExpModule::SignalPlotter
{
	void SignalPlotterData::ResetImpl(dispatch_tag<QMLModuleDataBase>)
	{
		Init();
	}

	void SignalPlotterData::Init()
	{
		Running = true;
		RollingView = false;
		Autoscale = true;

		PlotInfo = Graph::LineGraphPlotInfo();
		SampleDataList.clear();

		UIInitialized = false;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes SignalPlotter::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		std::vector<DynExpInstr::DataStreamBase::BasicSampleListType> BasicSamplesSeries;
		decltype(SignalPlotterData::SampleDataList) ProcessedSamples;
		bool Running{};
		Graph::LineGraphPlotInfo PlotInfo;

		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance.ModuleDataGetter());
			ProcessedSamples.resize(ModuleData->GetDataStreamInstrCount());
			Running = ModuleData->Running && !IsSavingData;
			PlotInfo = ModuleData->PlotInfo;

			if (Running)
			{
				PlotInfo.XUnit = DynExp::UnitType::Time_s;

				for (size_t i = 0; i < ProcessedSamples.size(); ++i)
				{
					ProcessedSamples[i].Visible = ModuleData->SampleDataList.size() == ProcessedSamples.size() ? ModuleData->SampleDataList[i].Visible : true;
					if (!ProcessedSamples[i].Visible)
					{
						BasicSamplesSeries.push_back({});
						continue;
					}

					ModuleData->GetDataStreamInstr(i)->ReadData();

					auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(ModuleData->GetDataStreamInstr(i)->GetInstrumentData());
					auto SampleStream = InstrData->GetSampleStream();

					if (!ModuleData->RollingView || !SampleStream->SeekEqual(std::ios_base::in))
						SampleStream->SeekBeg(std::ios_base::in);

					BasicSamplesSeries.emplace_back(SampleStream->ReadBasicSamples(SampleStream->GetStreamSizeRead()));
					PlotInfo.XUnit = (PlotInfo.XUnit == DynExp::UnitType::Time_s && SampleStream->IsBasicSampleTimeUsed()) ? DynExp::UnitType::Time_s : DynExp::UnitType::Index;
				}
			}

			NumFailedUpdateAttempts = 0;
		} // ModuleData unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		if (Running)
		{
			PlotInfo.Reset();
			PlotInfo.GenerateSampleTimingInfo(BasicSamplesSeries);
			for (size_t i = 0; i < ProcessedSamples.size(); ++i)
				 PlotInfo.ProcessBasicSamples(std::move(BasicSamplesSeries[i]), ProcessedSamples[i].Samples, i);
			PlotInfo.AdjustAxesLimits();

			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance.ModuleDataGetter());

				ModuleData->SampleDataList = std::move(ProcessedSamples);
				ModuleData->PlotInfo = std::move(PlotInfo);
			} // ModuleData unlocked here.
		}
		else
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance.ModuleDataGetter());

			PlotInfo.ResetHoveredSample();
			for (size_t i = 0; i < ProcessedSamples.size(); ++i)
				PlotInfo.ReprocessSamples(ModuleData->SampleDataList[i].Samples, i);

			ModuleData->PlotInfo = std::move(PlotInfo);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void SignalPlotter::ResetImpl(dispatch_tag<QMLModuleBase>)
	{
		IsSavingData = false;
		NumFailedUpdateAttempts = 0;
	}

	void SignalPlotter::MakeConnections(QObject* Backend)
	{
		QObject::connect(static_cast<SignalPlotterBackend*>(Backend), &SignalPlotterBackend::saveData, &SignalContext, [this]() { OnSaveData(); });

		Connect(static_cast<SignalPlotterBackend*>(Backend), &SignalPlotterBackend::rollingViewChanged, this, &SignalPlotter::OnRollingViewChanged);
		Connect(static_cast<SignalPlotterBackend*>(Backend), &SignalPlotterBackend::autoscaleChanged, this, &SignalPlotter::OnAutoscaleChanged);
		Connect(static_cast<SignalPlotterBackend*>(Backend), &SignalPlotterBackend::clearStream, this, &SignalPlotter::OnClearStream);
	}

	void SignalPlotter::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Backend = GetBackend<SignalPlotterBackend>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(ModuleDataGetter());

		// Initialize UI
		if (!ModuleData->IsUIInitialized())
		{
			Backend->SetRollingView(ModuleData->RollingView);
			Backend->SetAutoscale(ModuleData->Autoscale);

			for (const auto& InstrLabel : ModuleData->GetDataStreamInstrLabels())
				Backend->GetGraph()->InsertSeries(QString::fromStdString(InstrLabel));

			ModuleData->SetUIInitialized();
		}

		// Update plotted data
		ModuleData->Running = Backend->IsRunning();

		for (size_t i = 0; i < ModuleData->GetDataStreamInstrCount(); ++i)
		{
			auto& SampleData = ModuleData->SampleDataList.at(i);
			auto& Series = Backend->GetGraph()->UpdateSeries(i, SampleData.Samples, ModuleData->PlotInfo, ModuleData->Running);

			SampleData.Visible = Series.Visible;
		}

		// Update axes and hovered point
		Backend->GetGraph()->UpdateData(ModuleData->PlotInfo, ModuleData->Autoscale, ModuleData->Running);
	}

	void SignalPlotter::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<SignalPlotter>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleData->LockInstruments(Instance, ModuleParams->DataStreamInstr);

		ModuleData->PlotInfo.YUnit = ModuleData->GetDataStreamInstr(0)->GetValueUnit();
		for (size_t i = 1; i < ModuleData->GetDataStreamInstrCount(); ++i)
		{
			if (ModuleData->GetDataStreamInstr(0)->GetValueUnit() != ModuleData->PlotInfo.YUnit)
			{
				ModuleData->PlotInfo.YUnit = DynExp::UnitType::Arbitrary;
				break;
			}
		}

		ModuleData->RollingView = ModuleParams->RollingView.Get();
		ModuleData->Autoscale = ModuleParams->Autoscale.Get();
	}

	void SignalPlotter::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleData->UnlockInstruments(Instance);
	}

	void SignalPlotter::OnRollingViewChanged(DynExp::ModuleInstance* Instance, bool State) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<SignalPlotter>(GetNonConstParams());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleParams->RollingView = State;
		ModuleData->RollingView = State;
	}

	void SignalPlotter::OnAutoscaleChanged(DynExp::ModuleInstance* Instance, bool State) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<SignalPlotter>(GetNonConstParams());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleParams->Autoscale = State;
		ModuleData->Autoscale = State;
	}

	void SignalPlotter::OnClearStream(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		for (size_t i = 0; i < ModuleData->SampleDataList.size(); ++i)
		{
			if (ModuleData->SampleDataList[i].Visible)
			{
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(ModuleData->GetDataStreamInstr(i)->GetInstrumentData());

				InstrData->GetSampleStream()->Clear();
			}
		}
	}

	void SignalPlotter::OnSaveData() const
	{
		EnsureCallFromOwningThread();

		// As soon as FinishedSavingDataGuard is destroyed, IsSavingData is set back to false.
		FinishedSavingDataGuardType FinishedSavingDataGuard(*this, &SignalPlotter::FinishedSavingData);
		IsSavingData = true;

		auto Filename = Util::PromptSaveFilePathModule(GetWidget(), "Save data", ".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		std::stringstream CSVData;
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(GetModuleData());

			using SampleIteratorType = decltype(SignalPlotterData::SampleDataType::Samples)::const_iterator;
			std::vector<std::pair<SampleIteratorType, SampleIteratorType>> SeriesIterators;
			auto HeaderIterator = ModuleData->GetDataStreamInstrLabels().cbegin();
			const auto XUnit = std::string("_") + DynExp::UnitTypeToStr(ModuleData->PlotInfo.XUnit);
			const auto YUnit = std::string("_") + DynExp::UnitTypeToStr(ModuleData->PlotInfo.YUnit);
			
			for (const auto& Series : ModuleData->SampleDataList)
			{
				std::string Header(*HeaderIterator++);

				if (!Series.Visible)
					continue;

				SeriesIterators.emplace_back(Series.Samples.cbegin(), Series.Samples.cend());

				Header = Object::RemoveCategoryAndName(Header);
				std::erase_if(Header, [](char c) { return c == ' ' || c == '_' || c == ';'; });
				CSVData << "X_" << Header << XUnit << ";Y_" << Header << YUnit << ";";
			}

			CSVData << "\n";
			while (std::any_of(SeriesIterators.cbegin(), SeriesIterators.cend(), [](auto& IteratorPair) { return IteratorPair.first != IteratorPair.second; }))
			{
				for (auto& IteratorPair : SeriesIterators)
				{
					if (IteratorPair.first != IteratorPair.second)
					{
						CSVData << IteratorPair.first->x() / std::pow(10.0, ModuleData->PlotInfo.Multiplier) << ";" << IteratorPair.first->y() << ";";
						++IteratorPair.first;
					}
					else
						CSVData << ";;";
				}

				CSVData << "\n";
			}
		} // ModuleData unlocked here.

		if (!Util::SaveToFile(Filename, CSVData.str()))
			QMessageBox::warning(GetWidget(), "DynExp - Error", "Error writing data to file.");
	}
}