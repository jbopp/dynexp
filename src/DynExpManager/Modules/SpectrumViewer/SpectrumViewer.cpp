// This file is part of DynExp.

#include "stdafx.h"
#include "moc_SpectrumViewer.cpp"
#include "SpectrumViewer.h"

namespace DynExpModule::SpectrumViewer
{
	SpectrumViewerWidget::SpectrumViewerWidget(SpectrumViewer& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent),
		DataSeries(nullptr), DataChart(nullptr), XAxis(nullptr), YAxis(nullptr)
	{
		ui.setupUi(this);
		
		// For shortcuts
		this->addAction(ui.action_Run);
		this->addAction(ui.action_Stop);

		DataChart = new QChart();
		ui.Spectrum->setChart(DataChart);		// Takes ownership of DataChart.
		ui.Spectrum->setRenderHint(QPainter::Antialiasing);
		DataChart->setTheme(DynExpUI::DefaultQChartTheme);
		DataChart->legend()->setVisible(false);
	}

	void SpectrumViewerWidget::InitializeUI(Util::SynchronizedPointer<SpectrumViewerData>& ModuleData)
	{
		ui.SBExposureTime->setRange(ModuleData->MinExposureTime.count(), ModuleData->MaxExposureTime.count());
		ui.SBExposureTime->setSuffix(" " + QString::fromStdString(Util::ToUnitStr<DynExpInstr::SpectrometerData::TimeType>()));
		ui.SBLowerFrequency->setRange(ModuleData->MinFrequency, ModuleData->MaxFrequency);
		ui.SBLowerFrequency->setSuffix(" " + QString(DynExpInstr::SpectrometerData::FrequencyUnitTypeToStr(ModuleData->FrequencyUnit)));
		ui.SBUpperFrequency->setRange(ModuleData->MinFrequency, ModuleData->MaxFrequency);
		ui.SBUpperFrequency->setSuffix(" " + QString(DynExpInstr::SpectrometerData::FrequencyUnitTypeToStr(ModuleData->FrequencyUnit)));

		if (XAxis)
		{
			DataChart->removeAxis(XAxis);
			delete XAxis;
		}
		if (YAxis)
		{
			DataChart->removeAxis(YAxis);
			delete YAxis;
		}

		XAxis = new QValueAxis(this);
		YAxis = new QValueAxis(this);
		XAxis->setTitleText(QString("Frequency in ") + DynExpInstr::SpectrometerData::FrequencyUnitTypeToStr(ModuleData->FrequencyUnit));
		XAxis->setLabelFormat("%d");
		YAxis->setTitleText(QString("Intensity in ") + DynExpInstr::SpectrometerData::IntensityUnitTypeToStr(ModuleData->IntensityUnit));
		YAxis->setLabelFormat("%d");

		// Chart takes ownership of axes.
		DataChart->addAxis(XAxis, Qt::AlignBottom);
		DataChart->addAxis(YAxis, Qt::AlignLeft);
	}

	void SpectrumViewerWidget::UpdateUI(Util::SynchronizedPointer<SpectrumViewerData>& ModuleData)
	{
		ui.action_Save_CSV->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui.action_Run->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui.action_Stop->setEnabled(ModuleData->CapturingState == DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui.SBExposureTime->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui.SBLowerFrequency->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui.SBUpperFrequency->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);

		{
			const QSignalBlocker Blocker(ui.action_SilentMode);
			ui.action_SilentMode->setChecked(ModuleData->SilentModeEnabled);
		} // Blocker destroyed here.

		if (!ui.SBExposureTime->hasFocus())
		{
			const QSignalBlocker Blocker(ui.SBExposureTime);
			ui.SBExposureTime->setValue(ModuleData->CurrentExposureTime.count());
		}

		if (!ui.SBLowerFrequency->hasFocus())
		{
			const QSignalBlocker Blocker(ui.SBLowerFrequency);
			ui.SBLowerFrequency->setValue(ModuleData->CurrentLowerFrequency);
		}

		if (!ui.SBUpperFrequency->hasFocus())
		{
			const QSignalBlocker Blocker(ui.SBUpperFrequency);
			ui.SBUpperFrequency->setValue(ModuleData->CurrentUpperFrequency);
		}

		switch (ModuleData->CapturingState)
		{
		case DynExpInstr::SpectrometerData::CapturingStateType::Capturing:
			ui.LState->setText(" Acquiring spectrum...");
			ui.LState->setStyleSheet(DynExpUI::StatusBarBusyStyleSheet);
			break;
		case DynExpInstr::SpectrometerData::CapturingStateType::Warning:
			ui.LState->setText(" The spectrometer is in a warning state.");
			ui.LState->setStyleSheet(DynExpUI::StatusBarWarningStyleSheet);
			break;
		case DynExpInstr::SpectrometerData::CapturingStateType::Error:
			ui.LState->setText(" The spectrometer is in an error state.");
			ui.LState->setStyleSheet(DynExpUI::StatusBarErrorStyleSheet);
			break;
		default:
			ui.LState->setText(" Ready");
			ui.LState->setStyleSheet("");
		}

		ui.PBProgress->setVisible(ModuleData->CapturingState == DynExpInstr::SpectrometerData::CapturingStateType::Capturing
			&& ModuleData->CapturingProgress > 0);
		ui.PBProgress->setValue(ModuleData->CapturingProgress > 0 ? Util::NumToT<int>(ModuleData->CapturingProgress) : 0);
	}

	void SpectrumViewerWidget::SetData(SampleDataType&& SampleData, DynExpInstr::SpectrometerData::TimeType ExposureTime)
	{
		if (SampleData.Points.empty() || IsSavingData)
			return;

		CurrentSpectrum = std::move(SampleData);
		CurrentExposureTime = ExposureTime;

		// DataSeries->replace() does not work...
		DataChart->removeAllSeries();

		DataSeries = new QLineSeries(this);
		DataSeries->append(CurrentSpectrum.Points);
		DataSeries->setPointsVisible(false);

		DataChart->axes()[0]->setRange(CurrentSpectrum.MinValues.x(), CurrentSpectrum.MaxValues.x());
		if (CurrentSpectrum.MinValues.y() == CurrentSpectrum.MaxValues.y())
		{
			auto Factor = std::abs(CurrentSpectrum.MinValues.y()) * .01;
			if (Factor == 0)
				Factor = 2;

			DataChart->axes()[1]->setRange(CurrentSpectrum.MinValues.y() - Factor, CurrentSpectrum.MaxValues.y() + Factor);
		}
		else
			DataChart->axes()[1]->setRange(CurrentSpectrum.MinValues.y(), CurrentSpectrum.MaxValues.y());
		
		DataChart->addSeries(DataSeries);
		DataSeries->attachAxis(DataChart->axes()[0]);
		DataSeries->attachAxis(DataChart->axes()[1]);
	}

	void SpectrumViewerWidget::OnSaveCSVClicked()
	{
		// As soon as FinishedSavingDataGuard is destroyed, IsSavingData is set back to false.
		FinishedSavingDataGuardType FinishedSavingDataGuard(*this, &SpectrumViewerWidget::FinishedSavingData);
		IsSavingData = true;

		auto Filename = Util::PromptSaveFilePathModule(this, "Save data", ".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		if (!Util::SaveToFile(Filename, CurrentSpectrum.ToStr(CurrentExposureTime)))
			QMessageBox::warning(this, "DynExp - Error", "Error writing data to file.");
	}

	void SpectrumViewerWidget::SampleDataType::Reset()
	{
		Points.clear();

		MinValues = {};
		MaxValues = {};

		FrequencyUnit = DynExpInstr::SpectrometerData::FrequencyUnitType::Hz;
		IntensityUnit = DynExpInstr::SpectrometerData::IntensityUnitType::Counts;
	}

	std::string SpectrumViewerWidget::SampleDataType::ToStr(DynExpInstr::SpectrometerData::TimeType ExposureTime) const
	{
		std::stringstream CSVData;
		CSVData << std::setprecision(6);
		CSVData << "ExposureTime = " << ExposureTime.count() << " " << Util::ToUnitStr<DynExpInstr::SpectrometerData::TimeType>() << "\n";
		CSVData << "HEADER_END\n";

		CSVData << "f[" << DynExpInstr::SpectrometerData::FrequencyUnitTypeToStr(FrequencyUnit)
			<< "];I[" << DynExpInstr::SpectrometerData::IntensityUnitTypeToStr(IntensityUnit) << "]\n";
		for (const auto& Sample : Points)
			CSVData << Sample.x() << ";" << Sample.y() << "\n";

		return CSVData.str();
	}

	void SpectrumViewerData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void SpectrumViewerData::Init()
	{
		FrequencyUnit = DynExpInstr::SpectrometerData::FrequencyUnitType::Hz;
		IntensityUnit = DynExpInstr::SpectrometerData::IntensityUnitType::Counts;
		MinFrequency = 0.0;
		MaxFrequency = 0.0;
		MinExposureTime = DynExpInstr::SpectrometerData::TimeType();
		MaxExposureTime = DynExpInstr::SpectrometerData::TimeType();
		CurrentExposureTime = DynExpInstr::SpectrometerData::TimeType();
		AcquisitionExposureTime = DynExpInstr::SpectrometerData::TimeType();
		CurrentLowerFrequency = 0.0;
		CurrentUpperFrequency = 0.0;
		SilentModeEnabled = false;
		CapturingState = DynExpInstr::SpectrometerData::CapturingStateType::Ready;
		CapturingProgress = 0.0;
		AutoSaveFilename.clear();
		CurrentSpectrum.Reset();
		SpectrumRecordingPaused = false;

		UIInitialized = false;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes SpectrumViewer::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance.ModuleDataGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Spectrometer>(ModuleData->GetSpectrometer()->GetInstrumentData());

			ModuleData->CurrentExposureTime = InstrData->GetCurrentExposureTime();
			ModuleData->CurrentLowerFrequency = InstrData->GetCurrentLowerFrequency();
			ModuleData->CurrentUpperFrequency = InstrData->GetCurrentUpperFrequency();
			ModuleData->SilentModeEnabled = InstrData->GetSilentModeEnabled();
			ModuleData->CapturingState = InstrData->GetCapturingState();
			ModuleData->CapturingProgress = InstrData->GetCapturingProgress();

			if (InstrData->HasSpectrum() && !ModuleData->SpectrumRecordingPaused)
			{
				ModuleData->CurrentSpectrum = ProcessSpectrum(InstrData->GetSpectrum(), ModuleData);

				if (!ModuleData->CurrentSpectrum.Points.empty())
				{
					if (ModuleData->GetCommunicator().valid() && !ModuleData->AutoSaveFilename.empty())
						ModuleData->GetCommunicator()->PostEvent(*this, SpectrumFinishedRecordingEvent{});

					ModuleData->AutoSaveFilename.clear();
				}
			}

			NumFailedUpdateAttempts = 0;
		} // ModuleData and instruments' data unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void SpectrumViewer::ResetImpl(dispatch_tag<QModuleBase>)
	{
		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> SpectrumViewer::MakeUIWidget()
	{
		auto Widget = std::make_unique<SpectrumViewerWidget>(*this);

		Connect(Widget->GetUI().action_Run, &QAction::triggered, this, &SpectrumViewer::OnRunClicked);
		Connect(Widget->GetUI().action_Stop, &QAction::triggered, this, &SpectrumViewer::OnStopClicked);
		Connect(Widget->GetUI().action_SilentMode, &QAction::toggled, this, &SpectrumViewer::OnSilentModeToggled);
		Connect(Widget->GetUI().SBExposureTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpectrumViewer::OnExposureTimeChanged);
		Connect(Widget->GetUI().SBLowerFrequency, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpectrumViewer::OnLowerLimitChanged);
		Connect(Widget->GetUI().SBUpperFrequency, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpectrumViewer::OnUpperLimitChanged);

		return Widget;
	}

	void SpectrumViewer::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<SpectrumViewerWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(ModuleDataGetter());

		if (!ModuleData->IsUIInitialized())
		{
			Widget->InitializeUI(ModuleData);
			ModuleData->SetUIInitialized();
		}

		Widget->UpdateUI(ModuleData);

		if (!ModuleData->CurrentSpectrum.Points.empty())
			Widget->SetData(std::move(ModuleData->CurrentSpectrum), ModuleData->AcquisitionExposureTime);
	}

	SpectrumViewerWidget::SampleDataType SpectrumViewer::ProcessSpectrum(DynExpInstr::SpectrometerData::SpectrumType&& Spectrum,
		Util::SynchronizedPointer<SpectrumViewerData>& ModuleData)
	{
		SpectrumViewerWidget::SampleDataType TransformedSpectrum;
		TransformedSpectrum.FrequencyUnit = Spectrum.GetFrequencyUnit();
		TransformedSpectrum.IntensityUnit = Spectrum.GetIntensityUnit();

		if (!Spectrum.HasSpectrum())
			return TransformedSpectrum;

		double YMin(std::numeric_limits<double>::max()), YMax(std::numeric_limits<double>::lowest());
		for (const auto& Sample : Spectrum.GetSpectrum())
		{
			TransformedSpectrum.Points.append({ Sample.first, Sample.second });

			YMin = std::min(YMin, Sample.second);
			YMax = std::max(YMax, Sample.second);
		}

		TransformedSpectrum.MinValues = { Spectrum.GetSpectrum().begin()->first, YMin};
		TransformedSpectrum.MaxValues = { Spectrum.GetSpectrum().rbegin()->first, YMax};

		if (!ModuleData->AutoSaveFilename.empty())
			SaveSpectrum(TransformedSpectrum, ModuleData);

		return TransformedSpectrum;
	}

	void SpectrumViewer::SaveSpectrum(const SpectrumViewerWidget::SampleDataType& Spectrum,
		Util::SynchronizedPointer<SpectrumViewerData>& ModuleData)
	{
		if (!Util::SaveToFile(QString::fromStdString(ModuleData->AutoSaveFilename), Spectrum.ToStr(ModuleData->CurrentExposureTime)))
			Util::EventLogger().Log("Saving spectrum as \"" + ModuleData->AutoSaveFilename + "\" to file failed.", Util::ErrorType::Error);
		else
			Util::EventLogger().Log("Saved spectrum as \"" + ModuleData->AutoSaveFilename + "\" to file.");
	}

	void SpectrumViewer::OnInit(DynExp::ModuleInstance* Instance) const
	{
		RecordSpectrumEvent::Register(*this, &SpectrumViewer::OnRecordAndSaveSpectrum);
		PauseSpectrumRecordingEvent::Register(*this, &SpectrumViewer::OnPauseSpectrumRecording);
		ResumeSpectrumRecordingEvent::Register(*this, &SpectrumViewer::OnResumeSpectrumRecording);
		SetSilentModeEvent::Register(*this, &SpectrumViewer::OnSilentModeToggled);

		auto ModuleParams = DynExp::dynamic_Params_cast<SpectrumViewer>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->Spectrometer, ModuleData->GetSpectrometer());
		if (ModuleParams->Communicator.ContainsID())
			Instance->LockObject(ModuleParams->Communicator, ModuleData->GetCommunicator());

		ModuleData->FrequencyUnit = ModuleData->GetSpectrometer()->GetFrequencyUnit();
		ModuleData->IntensityUnit = ModuleData->GetSpectrometer()->GetIntensityUnit();
		ModuleData->MinFrequency = ModuleData->GetSpectrometer()->GetMinFrequency();
		ModuleData->MaxFrequency = ModuleData->GetSpectrometer()->GetMaxFrequency();

		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Spectrometer>(ModuleData->GetSpectrometer()->GetInstrumentData());
		ModuleData->MinExposureTime = InstrData->GetMinExposureTime();
		ModuleData->MaxExposureTime = InstrData->GetMaxExposureTime();
	}

	void SpectrumViewer::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->GetSpectrometer());
		Instance->UnlockObject(ModuleData->GetCommunicator());

		RecordSpectrumEvent::Deregister(*this);
		PauseSpectrumRecordingEvent::Deregister(*this);
		ResumeSpectrumRecordingEvent::Deregister(*this);
		SetSilentModeEvent::Deregister(*this);
	}

	void SpectrumViewer::OnRunClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

		ModuleData->SpectrumRecordingPaused = false;
		ModuleData->AcquisitionExposureTime = ModuleData->CurrentExposureTime;
		ModuleData->GetSpectrometer()->Record();
	}

	void SpectrumViewer::OnStopClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

		ModuleData->SpectrumRecordingPaused = false;
		ModuleData->AutoSaveFilename.clear();
		ModuleData->GetSpectrometer()->Abort();
	}

	void SpectrumViewer::OnSilentModeToggled(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

		ModuleData->GetSpectrometer()->SetSilentMode(Checked);
	}

	void SpectrumViewer::OnExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

		ModuleData->GetSpectrometer()->SetExposureTime(DynExpInstr::SpectrometerData::TimeType(Value));
	}

	void SpectrumViewer::OnLowerLimitChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Spectrometer>(ModuleData->GetSpectrometer()->GetInstrumentData());

		if (Value < InstrData->GetCurrentUpperFrequency())
			ModuleData->GetSpectrometer()->SetFrequencyRange(Value, InstrData->GetCurrentUpperFrequency());
	}

	void SpectrumViewer::OnUpperLimitChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Spectrometer>(ModuleData->GetSpectrometer()->GetInstrumentData());

		if (Value > InstrData->GetCurrentLowerFrequency())
			ModuleData->GetSpectrometer()->SetFrequencyRange(InstrData->GetCurrentLowerFrequency(), Value);
	}

	void SpectrumViewer::OnRecordAndSaveSpectrum(DynExp::ModuleInstance* Instance, std::string SaveDataFilename) const
	{
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

			if (ModuleData->CapturingState == DynExpInstr::SpectrometerData::CapturingStateType::Capturing)
				return;

			ModuleData->SpectrumRecordingPaused = false;
			ModuleData->AutoSaveFilename = SaveDataFilename;
		} // ModuleData unlocked here.

		OnRunClicked(Instance, false);
	}

	void SpectrumViewer::OnPauseSpectrumRecording(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());
		ModuleData->SpectrumRecordingPaused = true;
	}

	void SpectrumViewer::OnResumeSpectrumRecording(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());
		ModuleData->SpectrumRecordingPaused = false;
	}
}