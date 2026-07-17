// This file is part of DynExp.

#include "stdafx.h"
#include "SpectrumViewer.h"

#include "SpectrumViewerBackend.h"

namespace DynExpModule::SpectrumViewer
{
	std::string SpectrumViewerData::SampleDataType::ToStr(DynExpInstr::SpectrometerData::TimeType ExposureTime, const DynExpModule::Graph::LineGraphPlotInfo& PlotInfo) const
	{
		std::stringstream CSVData;
		CSVData << std::setprecision(6);
		CSVData << "ExposureTime = " << ExposureTime.count() << " " << Util::ToUnitStr<DynExpInstr::SpectrometerData::TimeType>() << "\n";
		CSVData << "HEADER_END\n";

		CSVData << "f[" << DynExp::Units::UnitTypeToStr(PlotInfo.XUnit) << "];I[" << DynExp::Units::UnitTypeToStr(PlotInfo.YUnit) << "]\n";
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
				Spectrum = InstrData->GetSpectrum();

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
			PlotInfo.ProcessSpectrum(std::move(Spectrum), ProcessedSamples.Samples, 0);
			PlotInfo.AdjustAxesLimits();

			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<SpectrumViewer>(Instance.ModuleDataGetter());

				ModuleData->CurrentSpectrum = std::move(ProcessedSamples);
				ModuleData->PlotInfo = std::move(PlotInfo);

				if (!ModuleData->AutoSaveFilename.empty())
				{
					SaveSpectrum(ModuleData);
					ModuleData->AutoSaveFilename.clear();

					if (ModuleData->GetCommunicator().valid())
						ModuleData->GetCommunicator()->PostEvent(*this, FinishedEvent{});
				}
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
			Backend->SetExposureTimeRange({ Util::NumToT<int>(ModuleData->MinExposureTime.count()), Util::NumToT<int>(ModuleData->MaxExposureTime.count()) });
			Backend->SetExposureTimeUnit(QString::fromStdString(Util::ToUnitStr<DynExpInstr::SpectrometerData::TimeType>()));
			Backend->SetLimitRange({ ModuleData->MinFrequency, ModuleData->MaxFrequency });
			Backend->SetLimitUnit(DynExp::Units::UnitTypeToStr(ModuleData->PlotInfo.XUnit));

			Backend->GetGraph()->InsertSeries("Spectrum");

			ModuleData->SetUIInitialized();
		}

		switch (ModuleData->CapturingState)
		{
		case DynExpInstr::SpectrometerData::CapturingStateType::Warning: Backend->SetState(SpectrumViewerBackend::Warning); break;
		case DynExpInstr::SpectrometerData::CapturingStateType::Error: Backend->SetState(SpectrumViewerBackend::Error); break;
		case DynExpInstr::SpectrometerData::CapturingStateType::Capturing: Backend->SetState(SpectrumViewerBackend::Capturing); break;
		default: Backend->SetState(SpectrumViewerBackend::Ready);
		}

		if (!Backend->IsSilentFocused())
			Backend->SetSilent(ModuleData->SilentModeEnabled);
		if (!Backend->IsExposureTimeFocused())
			Backend->SetExposureTime(Util::NumToT<int>(ModuleData->CurrentExposureTime.count()));
		if (!Backend->IsLowerLimitFocused())
			Backend->SetLowerLimit(ModuleData->CurrentLowerFrequency);
		if (!Backend->IsUpperLimitFocused())
			Backend->SetUpperLimit(ModuleData->CurrentUpperFrequency);
		Backend->SetProgress(ModuleData->CapturingProgress > 0. && ModuleData->CapturingProgress <= 100. ? ModuleData->CapturingProgress / 100. : 0.);

		Backend->GetGraph()->UpdateSeries(0, ModuleData->CurrentSpectrum.Samples, ModuleData->PlotInfo);
		Backend->GetGraph()->UpdateData(ModuleData->PlotInfo);
	}

	void SpectrumViewer::SaveSpectrum(Util::SynchronizedPointer<SpectrumViewerData>& ModuleData)
	{
		if (!Util::SaveToFile(QString::fromStdString(ModuleData->AutoSaveFilename), ModuleData->CurrentSpectrum.ToStr(ModuleData->CurrentExposureTime, ModuleData->PlotInfo)))
			Util::EventLog().Log("[SpectrumViewer] Saving spectrum as \"" + ModuleData->AutoSaveFilename + "\" to file failed.", Util::ErrorType::Error);
	}

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
		if (!Util::SaveToFile(Filename, ModuleData->CurrentSpectrum.ToStr(ModuleData->CurrentExposureTime, ModuleData->PlotInfo)))
			QMessageBox::warning(GetWidget(), "DynExp - Error", "Error writing data to file.");
	}
}