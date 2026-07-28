// This file is part of DynExp.

#include "stdafx.h"
#include "SignalPlotter.h"

#include "SignalPlotterBackend.h"

namespace DynExpModule
{
	QString SignalPlotterData::PlotInfoType::GetMultiplierLabel() const
	{
		switch (Multiplier)
		{
		case 0: return "";
		case 3: return "m";
		case 6: return "u";
		case 9: return "n";
		default: return "?";
		}
	}

	void SignalPlotterData::PlotInfoType::ResetHoveredSample()
	{
		HoveredPoint = {};
		HoveredSample = {};
		HoveredDistance = std::numeric_limits<typename SignalPlotterData::PlotInfoType::QPointFValueType>::max();
	}

	SignalPlotterData::SignalPlotterData()
	{
		Init();
	}

	void SignalPlotterData::ResetImpl(dispatch_tag<QMLModuleDataBase>)
	{
		Init();
	}

	void SignalPlotterData::Init()
	{
		UIInitialized = false;
		Running = true;
		RollingView = false;
		Autoscale = true;

		PlotInfo = PlotInfoType();
		SampleDataList.clear();
	}

	Util::DynExpErrorCodes::DynExpErrorCodes SignalPlotter::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		std::vector<DynExpInstr::DataStreamBase::BasicSampleListType> BasicSamplesSeries;
		decltype(SignalPlotterData::SampleDataList) ProcessedSamples;
		bool Running{};
		SignalPlotterData::PlotInfoType PlotInfo;

		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance.ModuleDataGetter());
			ProcessedSamples.resize(ModuleData->GetDataStreamInstrCount());
			Running = ModuleData->Running && !IsSavingData;
			PlotInfo = ModuleData->PlotInfo;
			PlotInfo.IsBasicSampleTimeUsed = true;

			if (Running)
			{
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
					PlotInfo.IsBasicSampleTimeUsed = PlotInfo.IsBasicSampleTimeUsed && SampleStream->IsBasicSampleTimeUsed();
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
			GenerateSampleTimingInfo(BasicSamplesSeries, PlotInfo);

			bool DataAvailable = false;
			for (size_t i = 0; i < ProcessedSamples.size(); ++i)
				DataAvailable = ProcessBasicSamples(BasicSamplesSeries[i], ProcessedSamples[i], PlotInfo, i) || DataAvailable;
			
			if (!DataAvailable)
			{
				PlotInfo.MinValues = { 0., 0. };
				PlotInfo.MaxValues = { 1., 1. };
			}
			else
			{
				if (std::all_of(ProcessedSamples.cbegin(), ProcessedSamples.cend(), [](const auto& x) { return x.Samples.size() == 1; }))
				{
					PlotInfo.MaxValues.setY(std::max(std::abs(PlotInfo.MinValues.y()), std::abs(PlotInfo.MaxValues.y())));
					PlotInfo.MinValues.setY(0.);
				}
				else if (PlotInfo.MinValues.y() == PlotInfo.MaxValues.y())
				{
					auto YRangeDelta = std::abs(PlotInfo.MinValues.y()) * .01;
					YRangeDelta = YRangeDelta == 0. ? 2. : YRangeDelta;
					PlotInfo.MinValues.setY(PlotInfo.MinValues.y() - YRangeDelta);
					PlotInfo.MaxValues.setY(PlotInfo.MaxValues.y() + YRangeDelta);
				}
			}

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
				ReprocessSamples(ModuleData->SampleDataList[i], PlotInfo, i);

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
				Backend->InsertSeries(QString::fromStdString(InstrLabel));

			ModuleData->SetUIInitialized();
		}

		// Update plotted data
		ModuleData->Running = Backend->IsRunning();
		ModuleData->PlotInfo.CursorPosition = Backend->GetCursorPosition();

		bool AnyLineSeriesVisible = false;
		QStringList BarSeriesNames;
		for (size_t i = 0; i < ModuleData->GetDataStreamInstrCount(); ++i)
		{
			auto& SampleData = ModuleData->SampleDataList.at(i);
			auto& Series = Backend->GetSeries(Util::NumToT<qsizetype>(i));

			if (Series.Visible)
			{
				if (ModuleData->Running)
				{
					if (!SampleData.Samples.empty())
						Series.BarSet->replace(0, std::abs(SampleData.Samples.front().y()));
					Series.LineSeries->replace(SampleData.Samples);
				}

				Series.LineSeries->setWidth(!ModuleData->PlotInfo.HoveredPoint.isNull() && ModuleData->PlotInfo.HoveredSeries == i ? 4 : 2);
			}

			SampleData.Visible = Series.Visible;
			Series.BarSeries->setVisible(Series.Visible && SampleData.Samples.size() == 1);
			Series.LineSeries->setVisible(Series.Visible && SampleData.Samples.size() > 1);
			AnyLineSeriesVisible = AnyLineSeriesVisible || Series.LineSeries->isVisible();
			BarSeriesNames.push_back(Series.Name);

			Backend->SeriesChanged(i);
		}

		// Update axes
		if (ModuleData->Running)
		{
			Backend->GetXCategoryAxis()->setVisible(!AnyLineSeriesVisible);
			Backend->GetXValueAxis()->setVisible(AnyLineSeriesVisible);
			if (AnyLineSeriesVisible)
			{
				Backend->GetXValueAxis()->setTitleText(ModuleData->PlotInfo.IsBasicSampleTimeUsed ? "time in " + ModuleData->PlotInfo.GetMultiplierLabel() + "s" : "sample in #");
				Backend->GetXValueAxis()->setLabelFormat(ModuleData->PlotInfo.IsBasicSampleTimeUsed ? "%.3f" : "%.0f");
				Backend->GetXValueAxis()->setRange(ModuleData->PlotInfo.MinValues.x(), ModuleData->PlotInfo.MaxValues.x());
			}
			else
				Backend->GetXCategoryAxis()->setCategories(BarSeriesNames);
			
			Backend->GetYValueAxis()->setTitleText(QString("signal in ") + DynExpInstr::DataStreamInstrumentData::UnitTypeToStr(ModuleData->PlotInfo.ValueUnit));
			if (ModuleData->PlotInfo.ValueUnit == DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel)
			{
				Backend->GetYValueAxis()->setLabelFormat("%.0f");
				if (ModuleData->Autoscale)
					Backend->GetYValueAxis()->setRange(0, 1);
				Backend->GetYValueAxis()->setTickInterval(1);
			}
			else
			{
				Backend->GetYValueAxis()->setLabelFormat("%.3f");
				if (ModuleData->Autoscale)
					Backend->GetYValueAxis()->setRange(ModuleData->PlotInfo.MinValues.y(), ModuleData->PlotInfo.MaxValues.y());
				Backend->GetYValueAxis()->setTickInterval(0);
			}

			Backend->UpdateAxes(AnyLineSeriesVisible);
		}

		if (AnyLineSeriesVisible)
		{
			Backend->SetHoveredPoint(ModuleData->PlotInfo.HoveredPoint);
			Backend->SetHoveredSample(ModuleData->PlotInfo.HoveredSample);
		}
	}

	void SignalPlotter::GenerateSampleTimingInfo(std::vector<DynExpInstr::DataStreamBase::BasicSampleListType>& BasicSamplesSeries,
		SignalPlotterData::PlotInfoType& PlotInfo)
	{
		bool TimingInfoFound = false;
		PlotInfo.Multiplier = std::numeric_limits<decltype(std::declval<SignalPlotterData::PlotInfoType>().Multiplier)>::max();
		PlotInfo.LastMinValues = PlotInfo.MinValues;
		PlotInfo.LastMaxValues = PlotInfo.MaxValues;
		PlotInfo.MinValues = {
			std::numeric_limits<typename SignalPlotterData::PlotInfoType::QPointFValueType>::max(),
			std::numeric_limits<typename SignalPlotterData::PlotInfoType::QPointFValueType>::max()
		};
		PlotInfo.MaxValues = {
			std::numeric_limits<typename SignalPlotterData::PlotInfoType::QPointFValueType>::lowest(),
			std::numeric_limits<typename SignalPlotterData::PlotInfoType::QPointFValueType>::lowest()
		};
		PlotInfo.ResetHoveredSample();

		if (PlotInfo.IsBasicSampleTimeUsed)
		{
			for (auto& Samples : BasicSamplesSeries)
			{
				if (Samples.empty())
					continue;

				// Use stable_sort() to not affect the order of samples with equal time, in case the data
				// stream supports sample timing but the user of the stream ignores it.
				std::stable_sort(Samples.begin(), Samples.end(), [](const auto& a, const auto& b) {
					return a.Time < b.Time;
				});

				TimingInfoFound = true;

				// Switch back to use sample indices as x values if all Time values are equal.
				if (Samples.front().Time == Samples.back().Time && Samples.size() > 1)
				{
					PlotInfo.IsBasicSampleTimeUsed = false;
					PlotInfo.Multiplier = 0;

					break;
				}

				// Determine best order of magnitude to display the time with.
				if (std::abs(Samples.front().Time) < 1e-6 && std::abs(Samples.back().Time) < 1e-6)
					PlotInfo.Multiplier = std::min(PlotInfo.Multiplier, 9u);
				else if (std::abs(Samples.front().Time) < 1e-3 && std::abs(Samples.back().Time) < 1e-3)
					PlotInfo.Multiplier = std::min(PlotInfo.Multiplier, 6u);
				else if (std::abs(Samples.front().Time) < 1.0 && std::abs(Samples.back().Time) < 1.0)
					PlotInfo.Multiplier = std::min(PlotInfo.Multiplier, 3u);
				else
					PlotInfo.Multiplier = 0;
			}
		}
		
		if (!TimingInfoFound)
			PlotInfo.Multiplier = 0;
	}

	bool SignalPlotter::ProcessBasicSamples(const DynExpInstr::DataStreamBase::BasicSampleListType& BasicSamples,
		SignalPlotterData::SampleDataType& SampleData, SignalPlotterData::PlotInfoType& PlotInfo, const size_t SeriesIndex)
	{
		if (BasicSamples.empty())
			return false;

		auto YMin{ std::numeric_limits<typename SignalPlotterData::PlotInfoType::QPointFValueType>::max() };
		auto YMax{ std::numeric_limits<typename SignalPlotterData::PlotInfoType::QPointFValueType>::lowest() };

		for (size_t i = 0; i < BasicSamples.size(); ++i)
		{
			const auto X = PlotInfo.IsBasicSampleTimeUsed ? BasicSamples[i].Time * std::pow(10.0, PlotInfo.Multiplier) : i;
			const auto Y = BasicSamples[i].Value;
			SampleData.Samples.append({ X, Y });

			YMin = std::min(YMin, Y);
			YMax = std::max(YMax, Y);

			// To avoid a second loop, do the calculation with axes limits from the previous run.
			// Find hovered point for series with more than a single sample.
			if (i)
				CheckSampleHovered(X, Y, PlotInfo, SeriesIndex);
		}

		PlotInfo.MinValues = { std::min(PlotInfo.MinValues.x(), SampleData.Samples.first().x()), std::min(PlotInfo.MinValues.y(), YMin) };
		PlotInfo.MaxValues = { std::max(PlotInfo.MaxValues.x(), SampleData.Samples.last().x()), std::max(PlotInfo.MaxValues.y(), YMax) };

		return true;
	}

	void SignalPlotter::ReprocessSamples(const SignalPlotterData::SampleDataType& SampleData,
		SignalPlotterData::PlotInfoType& PlotInfo, const size_t SeriesIndex)
	{
		for (size_t i = 0; i < Util::NumToT<size_t>(SampleData.Samples.size()); ++i)
		{
			// Find hovered point for series with more than a single sample.
			if (i)
				CheckSampleHovered(SampleData.Samples[i].x(), SampleData.Samples[i].y(), PlotInfo, SeriesIndex);
		}
	}

	void SignalPlotter::CheckSampleHovered(const typename SignalPlotterData::PlotInfoType::QPointFValueType X,
		const typename SignalPlotterData::PlotInfoType::QPointFValueType Y,
		SignalPlotterData::PlotInfoType& PlotInfo, const size_t SeriesIndex)
	{
		if (!(PlotInfo.LastMinValues.isNull() && PlotInfo.LastMaxValues.isNull()) && !PlotInfo.CursorPosition.isNull())
		{
			const auto XNormalized = (X - PlotInfo.LastMinValues.x()) / (PlotInfo.LastMaxValues.x() - PlotInfo.LastMinValues.x());
			const auto YNormalized = (Y - PlotInfo.LastMinValues.y()) / (PlotInfo.LastMaxValues.y() - PlotInfo.LastMinValues.y());
			const auto XDist = XNormalized - PlotInfo.CursorPosition.x();
			const auto YDist = YNormalized - PlotInfo.CursorPosition.y();
			const auto Dist = XDist * XDist + YDist * YDist;

			if (Dist < 0.001 && Dist < PlotInfo.HoveredDistance)
			{
				PlotInfo.HoveredDistance = Dist;
				PlotInfo.HoveredPoint = { XNormalized, YNormalized };
				PlotInfo.HoveredSample = { X, Y };
				PlotInfo.HoveredSeries = SeriesIndex;
			}
		}
	}

	void SignalPlotter::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<SignalPlotter>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleData->LockInstruments(Instance, ModuleParams->DataStreamInstr);

		ModuleData->PlotInfo.ValueUnit = ModuleData->GetDataStreamInstr(0)->GetValueUnit();
		for (size_t i = 1; i < ModuleData->GetDataStreamInstrCount(); ++i)
		{
			if (ModuleData->GetDataStreamInstr(0)->GetValueUnit() != ModuleData->PlotInfo.ValueUnit)
			{
				ModuleData->PlotInfo.ValueUnit = DynExpInstr::DataStreamInstrumentData::UnitType::Arbitrary;
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
			const auto XUnit = ModuleData->PlotInfo.IsBasicSampleTimeUsed ? "_s" : "_i";
			const auto YUnit = std::string("_") + DynExpInstr::DataStreamInstrumentData::UnitTypeToStr(ModuleData->PlotInfo.ValueUnit);
			
			for (const auto& Series : ModuleData->SampleDataList)
			{
				std::string Header(*HeaderIterator++);

				if (!Series.Visible)
					continue;

				SeriesIterators.emplace_back(Series.Samples.cbegin(), Series.Samples.cend());

				Header = std::regex_replace(Header, std::regex("(\\([^()]*\\))"), "");	// remove instrument category.
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