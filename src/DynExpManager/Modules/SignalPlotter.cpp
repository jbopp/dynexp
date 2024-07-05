// This file is part of DynExp.

#include "stdafx.h"
#include "moc_SignalPlotter.cpp"
#include "SignalPlotter.h"

namespace DynExpModule
{
	SignalPlotterWidget::SignalPlotterWidget(SignalPlotter& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent),
		PlotContextMenu(new QMenu(this)),
		DataSeries(nullptr), DataChart(nullptr), XAxis(nullptr), YAxis(nullptr)
	{
		ui.setupUi(this);

		PlotAutoscaleAction = PlotContextMenu->addAction("&Autoscale axes");
		PlotAutoscaleAction->setCheckable(true);
		PlotAutoscaleAction->setChecked(true);
		PlotRollingViewAction = PlotContextMenu->addAction("&Rolling view");
		PlotRollingViewAction->setCheckable(true);
		PlotContextMenu->addSeparator();
		PlotClearAction = PlotContextMenu->addAction("&Clear stream");
		
		DataChart = new QChart();
		ui.Signal->setChart(DataChart);		// Takes ownership of DataChart.
		ui.Signal->setRenderHint(QPainter::Antialiasing);
		DataChart->setTheme(DynExpUI::DefaultQChartTheme);
		DataChart->legend()->setVisible(false);
		ui.action_Run->setChecked(true);
	}

	void SignalPlotterWidget::UpdateUI(bool IsRunning)
	{
		if (DataChart->axes().size() >= 2)
			DataChart->axes()[1]->setLabelsEditable(!IsRunning);
	}

	void SignalPlotterWidget::SetAxes(QValueAxis* XAxis, QValueAxis* YAxis)
	{
		if (!XAxis || !YAxis)
			return;

		if (this->XAxis)
		{
			DataChart->removeAxis(this->XAxis);
			delete this->XAxis;
		}
		if (this->YAxis)
		{
			DataChart->removeAxis(this->YAxis);
			delete this->YAxis;
		}

		this->XAxis = XAxis;
		this->YAxis = YAxis;

		// Chart takes ownership of axes.
		DataChart->addAxis(XAxis, Qt::AlignBottom);
		DataChart->addAxis(YAxis, Qt::AlignLeft);
	}

	void SignalPlotterWidget::SetData(const SignalPlotterWidget::SampleDataType& SampleData)
	{
		if (IsSavingData)
			return;

		// DataSeries->replace() does not work...
		DataChart->removeAllSeries();

		if (SampleData.Points.empty())
			return;

		if (SampleData.Points.size() > 1)
		{
			DataSeries = new QLineSeries(this);
			DataSeries->append(SampleData.Points);
			DataSeries->setPointsVisible(false);

			DataChart->axes()[0]->setRange(SampleData.MinValues.x(), SampleData.MaxValues.x());
		}
		else
		{
			auto ScatterSeries = new QScatterSeries(this);
			ScatterSeries->append(SampleData.Points);
			ScatterSeries->setMarkerShape(QScatterSeries::MarkerShape::MarkerShapeCircle);
			ScatterSeries->setMarkerSize(15);
			DataSeries = ScatterSeries;

			DataChart->axes()[0]->setRange(SampleData.MinValues.x() - 2, SampleData.MaxValues.x() + 2);
		}

		if (PlotAutoscaleAction->isChecked())
		{
			if (SampleData.MinValues.y() == SampleData.MaxValues.y())
			{
				auto Factor = std::abs(SampleData.MinValues.y()) * .01;
				if (Factor == 0)
					Factor = 2;

				DataChart->axes()[1]->setRange(SampleData.MinValues.y() - Factor, SampleData.MaxValues.y() + Factor);
			}
			else
				DataChart->axes()[1]->setRange(SampleData.MinValues.y(), SampleData.MaxValues.y());
		}
		
		DataChart->addSeries(DataSeries);
		DataSeries->attachAxis(DataChart->axes()[0]);
		DataSeries->attachAxis(DataChart->axes()[1]);

		Multiplier = SampleData.Multiplier;
	}

	void SignalPlotterWidget::OnPlotContextMenuRequested(const QPoint& Position)
	{
		PlotContextMenu->exec(ui.Signal->mapToGlobal(Position));
	}

	void SignalPlotterWidget::OnSaveCSVClicked()
	{
		// As soon as FinishedSavingDataGuard is destroyed, IsSavingData is set back to false.
		FinishedSavingDataGuardType FinishedSavingDataGuard(*this, &SignalPlotterWidget::FinishedSavingData);
		IsSavingData = true;

		auto Filename = Util::PromptSaveFilePathModule(this, "Save data", ".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		std::stringstream CSVData;
		CSVData << "X;Y\n";
		for (int i = 0; i < Util::NumToT<int>(DataSeries->count()); ++i)
			CSVData << DataSeries->at(i).x() / std::pow(10.0, Multiplier) << ";" << DataSeries->at(i).y() << "\n";

		if (!Util::SaveToFile(Filename, CSVData.str()))
			QMessageBox::warning(this, "DynExp - Error", "Error writing data to file.");
	}

	void SignalPlotterWidget::SampleDataType::Reset()
	{
		Points.clear();

		MinValues = {};
		MaxValues = {};
		Multiplier = 0;
	}

	QString SignalPlotterWidget::SampleDataType::GetMultiplierLabel() const
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

	void SignalPlotterData::SetCurrentSourceIndex(int Index)
	{
		// Update CurrentSourceIndex first since GetDataStreamInstr() depends on it.
		CurrentSourceIndex = Index;

		ValueUnit = GetDataStreamInstr()->GetValueUnit();
		UIInitialized = false;
	}

	void SignalPlotterData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void SignalPlotterData::Init()
	{
		UIInitialized = false;
		CurrentSourceIndex = 0;		// non-optional DataStreamInstr parameter, so it is ensured that there is at least one source (see RunnableInstance::LockObject()).
		IsRunning = true;
		IsBasicSampleTimeUsed = false;
		ValueUnit = DynExpInstr::DataStreamInstrumentData::UnitType::Arbitrary;
		PlotAxesChanged = true;
		Autoscale = true;
		RollingView = false;

		SampleData.Reset();
	}

	Util::DynExpErrorCodes::DynExpErrorCodes SignalPlotter::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		std::vector<DynExpInstr::BasicSample> BasicSamples;
		bool IsBasicSampleTimeUsed{};

		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance.ModuleDataGetter());

			if (ModuleData->IsRunning)
			{
				ModuleData->GetDataStreamInstr()->ReadData();

				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(ModuleData->GetDataStreamInstr()->GetInstrumentData());
				auto SampleStream = InstrData->GetSampleStream();

				if (!ModuleData->RollingView || !SampleStream->SeekEqual(std::ios_base::in))
					SampleStream->SeekBeg(std::ios_base::in);
				BasicSamples = SampleStream->ReadBasicSamples(SampleStream->GetStreamSizeRead());
				IsBasicSampleTimeUsed = SampleStream->IsBasicSampleTimeUsed();
			} // InstrData data unlocked here.

			NumFailedUpdateAttempts = 0;
		} // ModuleData unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		if (ProcessBasicSamples(std::move(BasicSamples), IsBasicSampleTimeUsed))
			IsBasicSampleTimeUsed = false;

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance.ModuleDataGetter());

			if (ModuleData->IsBasicSampleTimeUsed != IsBasicSampleTimeUsed && ModuleData->IsRunning)
			{
				ModuleData->IsBasicSampleTimeUsed = IsBasicSampleTimeUsed;
				ModuleData->PlotAxesChanged = true;
			}
		} // ModuleData unlocked here.

		return Util::DynExpErrorCodes::NoError;
	}

	void SignalPlotter::ResetImpl(dispatch_tag<QModuleBase>)
	{
		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> SignalPlotter::MakeUIWidget()
	{
		auto Widget = std::make_unique<SignalPlotterWidget>(*this);

		Connect(Widget->ui.action_Run, &QAction::triggered, this, &SignalPlotter::OnRunClicked);
		Connect(Widget->ui.CBSource, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SignalPlotter::OnSourceChanged);
		Connect(Widget->GetAutoscalePlotAction(), &QAction::triggered, this, &SignalPlotter::OnPlotAutoscaleClicked);
		Connect(Widget->GetRollingViewPlotAction(), &QAction::triggered, this, &SignalPlotter::OnPlotRollingViewClicked);
		Connect(Widget->GetClearPlotAction(), &QAction::triggered, this, &SignalPlotter::OnClearStream);

		return Widget;
	}

	void SignalPlotter::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<SignalPlotterWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(ModuleDataGetter());

		if (!Widget->ui.CBSource->count())
		{
			const QSignalBlocker CBSourceBlocker(Widget->ui.CBSource);

			for (const auto& InstrLabel : ModuleData->GetDataStreamInstrLabels())
				Widget->ui.CBSource->addItem(QIcon(ModuleData->GetDataStreamInstrIconPath().data()), QString::fromStdString(InstrLabel));

			Widget->ui.CBSource->setCurrentIndex(0);
			Widget->ui.CBSource->setVisible(Widget->ui.CBSource->count() > 1);
		}

		if (!ModuleData->IsUIInitialized())
		{
			Widget->GetAutoscalePlotAction()->setChecked(ModuleData->Autoscale);
			Widget->GetRollingViewPlotAction()->setChecked(ModuleData->RollingView);

			auto* XAxis = new QValueAxis(Widget);
			auto* YAxis = new QValueAxis(Widget);
			YAxis->setTitleText(QString("Signal in ") + DynExpInstr::DataStreamInstrumentData::UnitTypeToStr(ModuleData->ValueUnit));
			if (ModuleData->ValueUnit == DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel)
			{
				YAxis->setLabelFormat("%d");
				YAxis->setRange(0, 1);
				YAxis->setTickCount(2);
				YAxis->setMinorTickCount(0);
				YAxis->setMinorGridLineVisible(false);
			}
			
			Widget->SetAxes(XAxis, YAxis);

			ModuleData->PlotAxesChanged = true;
			ModuleData->SetUIInitialized();
		}

		if (ModuleData->PlotAxesChanged ||
			(ModuleData->IsRunning && ModuleData->SampleData.Multiplier != Widget->GetMultiplier() && !ModuleData->SampleData.Points.empty()))
		{
			if (Widget->GetXAxis())
			{
				Widget->GetXAxis()->setTitleText(ModuleData->IsBasicSampleTimeUsed ?
					"time in " + ModuleData->SampleData.GetMultiplierLabel() + "s" : "sample in #");
				Widget->GetXAxis()->setLabelFormat(ModuleData->IsBasicSampleTimeUsed ? "%.3f" : "%d");
			}

			ModuleData->PlotAxesChanged = false;
		}

		if (ModuleData->IsRunning)
		{
			Widget->SetData(ModuleData->SampleData);
			Widget->ui.LNumSamples->setText(QString::number(ModuleData->SampleData.Points.size()) + " sample"
				+ (ModuleData->SampleData.Points.size() == 1 ? "" : "s"));
		}

		Widget->UpdateUI(ModuleData->IsRunning);
	}

	bool SignalPlotter::ProcessBasicSamples(std::vector<DynExpInstr::BasicSample>&& BasicSamples, bool IsBasicSampleTimeUsed)
	{
		bool FallenBackToNotUseSampleTime = false;
		unsigned int Multiplier = 0;

		if (BasicSamples.empty())
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(GetModuleData());
			ModuleData->SampleData.Reset();

			return FallenBackToNotUseSampleTime;
		}

		// Sorting is believed to be very fast if samples are already sorted by ascending x values.
		if (IsBasicSampleTimeUsed)
		{
			// Use stable_sort() to not affect the order of samples with equal time, in case the data
			// stream supports sample timing, but the user of the stream ignores sample timing.
			std::stable_sort(BasicSamples.begin(), BasicSamples.end(), [](const auto& a, const auto& b) {
				return a.Time < b.Time;
			});

			// Switch back to use sample indices as x values if all Time values are equal.
			if (BasicSamples.front().Time == BasicSamples.back().Time && BasicSamples.size() > 1)
			{
				IsBasicSampleTimeUsed = false;
				FallenBackToNotUseSampleTime = true;
			}

			// Determine best order of magnitude to display the time with.
			if (std::abs(BasicSamples.front().Time) < 1.0 && std::abs(BasicSamples.back().Time) < 1.0)
			{
				Multiplier = 3;
				if (std::abs(BasicSamples.front().Time) < 1e-3 && std::abs(BasicSamples.back().Time) < 1e-3)
				{
					Multiplier = 6;
					if (std::abs(BasicSamples.front().Time) < 1e-6 && std::abs(BasicSamples.back().Time) < 1e-6)
						Multiplier = 9;
				}
			}
		}

		SignalPlotterWidget::SampleDataType SampleData;
		double YMin(std::numeric_limits<double>::max()), YMax(std::numeric_limits<double>::lowest());

		for (size_t i = 0; i < BasicSamples.size(); ++i)
		{
			auto Y = BasicSamples[i].Value;
			SampleData.Points.append({ IsBasicSampleTimeUsed ? BasicSamples[i].Time * std::pow(10.0, Multiplier) : i, Y });

			YMin = std::min(YMin, Y);
			YMax = std::max(YMax, Y);
		}

		SampleData.MinValues = { SampleData.Points.first().x(), YMin };
		SampleData.MaxValues = { SampleData.Points.last().x(), YMax };
		SampleData.Multiplier = Multiplier;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(GetModuleData());
		ModuleData->SampleData = std::move(SampleData);

		return FallenBackToNotUseSampleTime;
	}

	void SignalPlotter::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<SignalPlotter>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleData->LockInstruments(Instance, ModuleParams->DataStreamInstr);

		ModuleData->ValueUnit = ModuleData->GetDataStreamInstr()->GetValueUnit();
		ModuleData->Autoscale = ModuleParams->Autoscale.Get();
		ModuleData->RollingView = ModuleParams->RollingView.Get();
	}

	void SignalPlotter::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleData->UnlockInstruments(Instance);
	}

	void SignalPlotter::OnRunClicked(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		if (Checked)
			ModuleData->GetDataStreamInstr()->ResetStreamSize();
		
		ModuleData->IsRunning = Checked;
	}

	void SignalPlotter::OnSourceChanged(DynExp::ModuleInstance* Instance, int Index) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleData->SetCurrentSourceIndex(Index);
	}

	void SignalPlotter::OnPlotAutoscaleClicked(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<SignalPlotter>(GetNonConstParams());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleParams->Autoscale = Checked;
		ModuleData->Autoscale = Checked;
	}

	void SignalPlotter::OnPlotRollingViewClicked(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<SignalPlotter>(GetNonConstParams());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());

		ModuleParams->RollingView = Checked;
		ModuleData->RollingView = Checked;
	}

	void SignalPlotter::OnClearStream(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalPlotter>(Instance->ModuleDataGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(ModuleData->GetDataStreamInstr()->GetInstrumentData());
		
		InstrData->GetSampleStream()->Clear();
	}
}