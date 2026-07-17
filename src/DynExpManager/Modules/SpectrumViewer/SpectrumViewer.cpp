// This file is part of DynExp.

#include "stdafx.h"
#include "SpectrumViewer.h"

#include "SpectrumViewerBackend.h"

namespace DynExpModule::SpectrumViewer
{
	/*void SpectrumViewerWidget::InitializeUI(Util::SynchronizedPointer<SpectrumViewerData>& ModuleData)
	{
		ui->SBExposureTime->setRange(ModuleData->MinExposureTime.count(), ModuleData->MaxExposureTime.count());
		ui->SBExposureTime->setSuffix(" " + QString::fromStdString(Util::ToUnitStr<DynExpInstr::SpectrometerData::TimeType>()));
		ui->SBLowerFrequency->setRange(ModuleData->MinFrequency, ModuleData->MaxFrequency);
		ui->SBLowerFrequency->setSuffix(" " + QString(DynExpInstr::SpectrometerData::FrequencyUnitTypeToStr(ModuleData->FrequencyUnit)));
		ui->SBUpperFrequency->setRange(ModuleData->MinFrequency, ModuleData->MaxFrequency);
		ui->SBUpperFrequency->setSuffix(" " + QString(DynExpInstr::SpectrometerData::FrequencyUnitTypeToStr(ModuleData->FrequencyUnit)));

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
	}*/

	/*void SpectrumViewerWidget::UpdateUI(Util::SynchronizedPointer<SpectrumViewerData>& ModuleData)
	{
		ui->action_Save_CSV->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui->action_Run->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui->action_Stop->setEnabled(ModuleData->CapturingState == DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui->SBExposureTime->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui->SBLowerFrequency->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);
		ui->SBUpperFrequency->setEnabled(ModuleData->CapturingState != DynExpInstr::SpectrometerData::CapturingStateType::Capturing);

		{
			const QSignalBlocker Blocker(ui->action_SilentMode);
			ui->action_SilentMode->setChecked(ModuleData->SilentModeEnabled);
		} // Blocker destroyed here.

		if (!ui->SBExposureTime->hasFocus())
		{
			const QSignalBlocker Blocker(ui->SBExposureTime);
			ui->SBExposureTime->setValue(ModuleData->CurrentExposureTime.count());
		}

		if (!ui->SBLowerFrequency->hasFocus())
		{
			const QSignalBlocker Blocker(ui->SBLowerFrequency);
			ui->SBLowerFrequency->setValue(ModuleData->CurrentLowerFrequency);
		}

		if (!ui->SBUpperFrequency->hasFocus())
		{
			const QSignalBlocker Blocker(ui->SBUpperFrequency);
			ui->SBUpperFrequency->setValue(ModuleData->CurrentUpperFrequency);
		}

		switch (ModuleData->CapturingState)
		{
		case DynExpInstr::SpectrometerData::CapturingStateType::Capturing:
			ui->LState->setText(" Acquiring spectrum...");
			ui->LState->setStyleSheet(DynExpUI::StatusBarBusyStyleSheet);
			break;
		case DynExpInstr::SpectrometerData::CapturingStateType::Warning:
			ui->LState->setText(" The spectrometer is in a warning state.");
			ui->LState->setStyleSheet(DynExpUI::StatusBarWarningStyleSheet);
			break;
		case DynExpInstr::SpectrometerData::CapturingStateType::Error:
			ui->LState->setText(" The spectrometer is in an error state.");
			ui->LState->setStyleSheet(DynExpUI::StatusBarErrorStyleSheet);
			break;
		default:
			ui->LState->setText(" Ready");
			ui->LState->setStyleSheet("");
		}

		ui->PBProgress->setVisible(ModuleData->CapturingState == DynExpInstr::SpectrometerData::CapturingStateType::Capturing
			&& ModuleData->CapturingProgress > 0);
		ui->PBProgress->setValue(ModuleData->CapturingProgress > 0 ? Util::NumToT<int>(ModuleData->CapturingProgress) : 0);
	}*/

	std::string SpectrumViewerData::SampleDataType::ToStr(DynExpInstr::SpectrometerData::TimeType ExposureTime, const DynExpModule::Graph::LineGraphPlotInfo& PlotInfo) const
	{
		std::stringstream CSVData;
		CSVData << std::setprecision(6);
		CSVData << "ExposureTime = " << ExposureTime.count() << " " << Util::ToUnitStr<DynExpInstr::SpectrometerData::TimeType>() << "\n";
		CSVData << "HEADER_END\n";

		//CSVData << "f[" << DynExp::UnitTypeToStr(FrequencyUnit)
		//	<< "];I[" << DynExp::UnitTypeToStr(IntensityUnit) << "]\n";
		for (const auto& Sample : Samples)
			CSVData << Sample.x() << ";" << Sample.y() << "\n";

		return CSVData.str();
	}

	void SpectrumViewerData::ResetImpl(dispatch_tag<QMLModuleDataBase>)
	{
		Init();
	}

	void SpectrumViewerData::Init()
	{
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
		SpectrumRecordingPaused = false;

		PlotInfo = Graph::LineGraphPlotInfo();
		CurrentSpectrum = SampleDataType();

		UIInitialized = false;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes SpectrumViewer::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		DynExpInstr::SpectrometerData::SpectrumType Spectrum;
		SpectrumViewerData::SampleDataType ProcessedSamples;
		Graph::LineGraphPlotInfo PlotInfo;

		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance.ModuleDataGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Spectrometer>(ModuleData->GetSpectrometer()->GetInstrumentData());

			PlotInfo = ModuleData->PlotInfo;

			ModuleData->CurrentExposureTime = InstrData->GetCurrentExposureTime();
			ModuleData->CurrentLowerFrequency = InstrData->GetCurrentLowerFrequency();
			ModuleData->CurrentUpperFrequency = InstrData->GetCurrentUpperFrequency();
			ModuleData->SilentModeEnabled = InstrData->GetSilentModeEnabled();
			ModuleData->CapturingState = InstrData->GetCapturingState();
			ModuleData->CapturingProgress = InstrData->GetCapturingProgress();

			if (InstrData->HasSpectrum() && !ModuleData->SpectrumRecordingPaused)
			{
				Spectrum = InstrData->GetSpectrum();

				if (Spectrum.HasSpectrum() && !ModuleData->AutoSaveFilename.empty())
				{
					if (ModuleData->GetCommunicator().valid())
						ModuleData->GetCommunicator()->PostEvent(*this, FinishedEvent{});

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

		if (Spectrum.HasSpectrum())
		{
			PlotInfo.Reset();
			//PlotInfo.ProcessSpectrum(std::move(Spectrum), ProcessedSamples.Samples, 0);
			PlotInfo.AdjustAxesLimits();

			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance.ModuleDataGetter());

				ModuleData->CurrentSpectrum = std::move(ProcessedSamples);
				ModuleData->PlotInfo = std::move(PlotInfo);
			} // ModuleData unlocked here.
		}
		else
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance.ModuleDataGetter());

			PlotInfo.ResetHoveredSample();
			PlotInfo.ReprocessSamples(ModuleData->CurrentSpectrum.Samples, 0);

			ModuleData->PlotInfo = std::move(PlotInfo);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void SpectrumViewer::ResetImpl(dispatch_tag<QMLModuleBase>)
	{
		IsSavingData = false;
		NumFailedUpdateAttempts = 0;
	}

	void SpectrumViewer::MakeConnections(QObject* Backend)
	{
		QObject::connect(static_cast<SpectrumViewerBackend*>(Backend), &SpectrumViewerBackend::saveData, &SignalContext, [this]() { OnSaveSpectrum(); });

		Connect(static_cast<SpectrumViewerBackend*>(Backend), &SpectrumViewerBackend::runClicked, this, &SpectrumViewer::OnRunClicked);
		Connect(static_cast<SpectrumViewerBackend*>(Backend), &SpectrumViewerBackend::stopClicked, this, &SpectrumViewer::OnStopClicked);
		Connect(static_cast<SpectrumViewerBackend*>(Backend), &SpectrumViewerBackend::silentChanged, this, &SpectrumViewer::OnSilentModeToggled);
		Connect(static_cast<SpectrumViewerBackend*>(Backend), &SpectrumViewerBackend::exposureTimeChanged, this, &SpectrumViewer::OnExposureTimeChanged);
		Connect(static_cast<SpectrumViewerBackend*>(Backend), &SpectrumViewerBackend::lowerLimitChanged, this, &SpectrumViewer::OnLowerLimitChanged);
		Connect(static_cast<SpectrumViewerBackend*>(Backend), &SpectrumViewerBackend::upperLimitChanged, this, &SpectrumViewer::OnUpperLimitChanged);
	}

	void SpectrumViewer::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Backend = GetBackend<SpectrumViewerBackend>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(ModuleDataGetter());

		if (!ModuleData->IsUIInitialized())
		{
			Backend->GetGraph()->InsertSeries("Spectrum");

			ModuleData->SetUIInitialized();
		}

		Backend->GetGraph()->UpdateSeries(0, ModuleData->CurrentSpectrum.Samples, ModuleData->PlotInfo);
		Backend->GetGraph()->UpdateData(ModuleData->PlotInfo);
	}

	/*SpectrumViewerWidget::SampleDataType SpectrumViewer::ProcessSpectrum(DynExpInstr::SpectrometerData::SpectrumType&& Spectrum,
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

		if (!TransformedSpectrum.Points.empty() && !ModuleData->AutoSaveFilename.empty())
			SaveSpectrum(TransformedSpectrum, ModuleData);

		return TransformedSpectrum;
	}*/

	void SpectrumViewer::OnInit(DynExp::ModuleInstance* Instance) const
	{
		SetFilenameEvent::Register(*this, &SpectrumViewer::OnSetFilename);
		TriggerEvent::Register(*this, &SpectrumViewer::OnTrigger);
		StopEvent::Register(*this, &SpectrumViewer::OnStop);
		PauseSpectrumRecordingEvent::Register(*this, &SpectrumViewer::OnPauseSpectrumRecording);
		ResumeSpectrumRecordingEvent::Register(*this, &SpectrumViewer::OnResumeSpectrumRecording);
		SetSilentModeEvent::Register(*this, &SpectrumViewer::OnSilentModeToggled);

		auto ModuleParams = DynExp::dynamic_Params_cast<SpectrumViewer>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->Spectrometer, ModuleData->GetSpectrometer());
		if (ModuleParams->Communicator.ContainsID())
			Instance->LockObject(ModuleParams->Communicator, ModuleData->GetCommunicator());

		ModuleData->PlotInfo.XUnit = ModuleData->GetSpectrometer()->GetFrequencyUnit();
		ModuleData->PlotInfo.YUnit = ModuleData->GetSpectrometer()->GetIntensityUnit();
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

		SetFilenameEvent::Deregister(*this);
		TriggerEvent::Deregister(*this);
		StopEvent::Deregister(*this);
		PauseSpectrumRecordingEvent::Deregister(*this);
		ResumeSpectrumRecordingEvent::Deregister(*this);
		SetSilentModeEvent::Deregister(*this);
	}

	void SpectrumViewer::OnRunClicked(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

		if (ModuleData->CapturingState == DynExpInstr::SpectrometerData::CapturingStateType::Capturing)
			return;

		ModuleData->SpectrumRecordingPaused = false;
		ModuleData->AcquisitionExposureTime = ModuleData->CurrentExposureTime;
		ModuleData->GetSpectrometer()->Record();
	}

	void SpectrumViewer::OnStopClicked(DynExp::ModuleInstance* Instance) const
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

	void SpectrumViewer::OnSetFilename(DynExp::ModuleInstance* Instance, const std::string& SaveFilename) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance->ModuleDataGetter());

		OnStop(Instance);
		ModuleData->AutoSaveFilename = SaveFilename + ".csv";
	}

	void SpectrumViewer::OnTrigger(DynExp::ModuleInstance* Instance) const
	{
		OnRunClicked(Instance);
	}

	void SpectrumViewer::OnStop(DynExp::ModuleInstance* Instance) const
	{
		OnStopClicked(Instance);
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

	void SpectrumViewer::OnSaveSpectrum() const
	{
		EnsureCallFromOwningThread();

		// As soon as FinishedSavingDataGuard is destroyed, IsSavingData is set back to false.
		FinishedSavingDataGuardType FinishedSavingDataGuard(*this, &SpectrumViewer::FinishedSavingData);
		IsSavingData = true;

		auto Filename = Util::PromptSaveFilePathModule(GetWidget(), "Save data", ".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(GetModuleData());
		if (!Util::SaveToFile(QString::fromStdString(ModuleData->AutoSaveFilename), ModuleData->CurrentSpectrum.ToStr(ModuleData->CurrentExposureTime, ModuleData->PlotInfo)))
			Util::EventLog().Log("[SpectrumViewer] Saving spectrum as \"" + ModuleData->AutoSaveFilename + "\" to file failed.", Util::ErrorType::Error);
	}
}