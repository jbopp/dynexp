// This file is part of DynExp.

#include "stdafx.h"
#include "ui_ODMR.h"
#include "ODMR.h"

namespace DynExpModule::ODMR
{
	void ODMRData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	unsigned long long ODMRData::GetSweepNumberSteps() const noexcept
	{
		if (!SweepSeriesEnabled || SweepSeriesStep <= 0)
			return 0;

		return std::floor(std::abs(SweepSeriesStop - SweepSeriesStart) / SweepSeriesStep) + 1;
	}

	std::stringstream ODMRData::AssembleCSVHeader(double RFPower, double RFModulationDepth, double AuxAnalogOutValue, bool IsRFOffResonance)
	{
		std::stringstream CSVData;

		CSVData << std::setprecision(6);
		CSVData << "RFPower = " << RFPower << " " << GetRFGenerator()->GetValueUnitStr() << "\n";
		CSVData << "RFCenterFreq = " << RFCenterFreq << " Hz\n";
		CSVData << "RFFreqSpan = " << RFFreqSpan << " Hz\n";
		CSVData << "RFFreqSpacing = " << RFFreqSpacing << " Hz\n";
		CSVData << "RFDwellTime = " << RFDwellTime << " s\n";

		CSVData << "RFModulation = ";
		switch (RFModulation)
		{
		case RFModulationType::None: CSVData << "none\n"; break;
		case RFModulationType::Sine: CSVData << "sine\n"; break;
		case RFModulationType::Pulse: CSVData << "pulse\n"; break;
		default: CSVData << "\n";
		}
		CSVData << "RFModulationFreq = " << RFModulationFreq << " Hz\n";
		CSVData << "RFModulationDepth = " << RFModulationDepth << " Hz\n";

		CSVData << "ODMRSamplingRate = " << ODMRSamplingRate << " samples/s\n";
		CSVData << "CurrentSaveIndex = " << CurrentSaveIndex << "\n";

		CSVData << "IsRFOffResonance = " << (IsRFOffResonance ? "yes" : "no") << "\n";
		CSVData << "SensitivityEnabled = " << (SensitivityEnabled ? "yes" : "no") << "\n";
		CSVData << "SensitivityOncePerSweep = " << (SensitivityOncePerSweep ? "yes" : "no") << "\n";
		CSVData << "SensitivityOffResonanceEnabled = " << (SensitivityOffResonanceEnabled ? "yes" : "no") << "\n";
		CSVData << "SensitivityResonanceFreq = " << SensitivityResonanceFreq << " Hz\n";
		CSVData << "SensitivityOffResonanceFreq = " << SensitivityOffResonanceFreq << " Hz\n";
		CSVData << "SensitivityResonanceSpan = " << SensitivityResonanceSpan << " Hz\n";
		CSVData << "SensitivitySamplingRate = " << SensitivitySamplingRate << " samples/s\n";
		CSVData << "SensitivityDuration = " << SensitivityDuration << " s\n";

		CSVData << "SensitivityAnalysisEnabled = " << (SensitivityAnalysisEnabled ? "yes" : "no") << "\n";
		CSVData << "GyromagneticRatio = " << GyromagneticRatio << " Hz/T\n";

		CSVData << "SweepSeriesEnabled = " << (SweepSeriesEnabled ? "yes" : "no") << "\n";
		CSVData << "SweepSeries = ";
		switch (SweepSeries)
		{
		case SweepSeriesType::RFModulationDepth: CSVData << "RFModulationDepth\n"; break;
		case SweepSeriesType::RFPower: CSVData << "RFPower\n"; break;
		case SweepSeriesType::AnalogOut: CSVData << "AnalogOut\n"; break;
		default: CSVData << "\n";
		}
		CSVData << "SweepSeriesStart = " << SweepSeriesStart << "\n";
		CSVData << "SweepSeriesStop = " << SweepSeriesStop << "\n";
		CSVData << "SweepSeriesStep = " << SweepSeriesStep << "\n";
		CSVData << "SweepSeriesRetrace = " << (SweepSeriesRetrace ? "yes" : "no") << "\n";
		CSVData << "SweepSeriesAdvanceLastValue = " << (SweepSeriesAdvanceLastValue ? "yes" : "no") << "\n";
		CSVData << "CurrentSweepIndex = " << CurrentSweepIndex << "\n";
		CSVData << "AuxAnalogOutValue = " << AuxAnalogOutValue << "\n";

		if (TestFeature(ODMRData::FeatureType::LockinDetection))
		{
			auto& LockinInstr = dynamic_cast<const DynExpInstr::LockinAmplifier&>(*GetSignalDetector());
			auto LockinData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::LockinAmplifier>(GetSignalDetector()->GetInstrumentData());

			auto Sensitivity = LockinData->GetSensitivity();
			auto TimeConstant = LockinData->GetTimeConstant();
			unsigned int FilterOrder = LockinData->GetFilterOrder();

			CSVData << "LockinSensitivity = " << Sensitivity << " " << LockinInstr.GetSensitivityUnitString() << "\n";
			CSVData << "LockinTimeConstant = " << TimeConstant << " s\n";
			CSVData << "LockinFilterOrder = " << FilterOrder << "\n";
		}

		CSVData << "HEADER_END\n";

		return CSVData;
	}

	void ODMRData::Init()
	{
		RFPower = -10;
		RFAutoEnabled = true;
		RFCenterFreq = 2.87e9;
		RFFreqSpan = 100e6;
		RFFreqSpacing = 10e3;
		RFDwellTime = 10e-3;

		RFModulation = RFModulationType::None;
		RFModulationFreq = 10e3;
		RFModulationDepth = 1e6;

		ODMRSamplingRate = .1e3;

		MeasurementMode = MeasurementModeType::All;
		SaveDataPath = "";
		CurrentSaveIndex = 0;
		AutosaveEnabled = false;

		SensitivityEnabled = false;
		SensitivityOncePerSweep = false;
		SensitivityOffResonanceEnabled = false;
		SensitivityResonanceFreq = 2.87e9;
		SensitivityOffResonanceFreq = 2.8e9;
		SensitivityResonanceSpan = 10e6;
		SensitivitySamplingRate = 1e3;
		SensitivityDuration = 1;

		SensitivityAnalysisEnabled = true;
		GyromagneticRatio = 28e9;

		SweepSeriesEnabled = false;
		SweepSeries = SweepSeriesType::RFModulationDepth;
		SweepSeriesStart = 1e3;
		SweepSeriesStop = 10e3;
		SweepSeriesStep = .1e3;
		SweepSeriesRetrace = true;
		SweepSeriesAdvanceLastValue = false;
		CurrentSweepIndex = 0;

		RFGeneratorMinFuncDesc = {};
		RFGeneratorMaxFuncDesc = {};
		RFGeneratorDefaultFuncDesc = {};

		AuxAnalogOutValueUnit = DynExp::Units::UnitType::Arbitrary;
		AuxAnalogOutMinValue = 0.0;
		AuxAnalogOutMaxValue = 1.0;

		AcquisitionTime = 0.0;

		ODMRPlot = ODMRPlotType();
		ODMRPlot.PlotInfo.XUnit = DynExp::Units::UnitType::Freq_GHz;
		ODMRPlot.PlotInfo.YUnit = DynExp::Units::UnitType::Volt;
		ODMRPlot.PlotInfo.YLabel = "ODMR signal [V]";
		SensitivityPlot = SensitivityPlotType();
		SensitivityPlot.PlotInfo.XUnit = DynExp::Units::UnitType::Freq_Hz;
		SensitivityPlot.PlotInfo.YUnit = DynExp::Units::UnitType::TperSqrtHz;
		SensitivityPlot.PlotInfo.XIsLogarithmic = true;
		SensitivityPlot.PlotInfo.YIsLogarithmic = true;

		Features = {};
	}

	ODMR::ODMR::ODMR(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: QModuleBase(OwnerThreadID, std::move(Params)),
		StateMachine(InitializingState, ReadyState,
			MeasurementSeriesStepState, MeasurementSeriesInitState,
			ODMRTraceInitState, ODMRTraceWaitState, ODMRTraceFinishState,
			SensitivityInitState, SensitivityWaitState, SensitivityFinishState)
	{
	}

	std::chrono::milliseconds ODMR::ODMR::GetMainLoopDelay() const
	{
		auto CurrentState = StateMachine.GetCurrentState()->GetState();

		// If doing some measurement, run much faster.
		if (CurrentState == StateType::Ready)
			return std::chrono::milliseconds(30);
		else
			return std::chrono::milliseconds(2);
	}

	Util::DynExpErrorCodes::DynExpErrorCodes ODMR::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

			if (ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection))
			{
				auto LockinData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::LockinAmplifier>(ModuleData->GetSignalDetector()->GetInstrumentData());

				ModuleData->AcquisitionTime = LockinData->GetAcquisitionTime();
			} // LockinData unlocked here.

			StateMachine.Invoke(*this, Instance);

			NumFailedUpdateAttempts = 0;
		}
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void ODMR::ResetImpl(dispatch_tag<QModuleBase>)
	{
		StateMachine.SetCurrentState(StateType::Initializing);

		NextRFPower = 0;
		NextRFModulationDepth = 0;
		NextAuxAnalogOutValue = 0;

		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> ODMR::MakeUIWidget()
	{
		auto Widget = std::make_unique<ODMRWidget>(*this);

		Connect(Widget->GetUI()->SBRFPower, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnRFPowerChanged);
		Connect(Widget->GetUI()->CBRFAutoEnable, &QCheckBox::checkStateChanged, this, &ODMR::OnRFAutoEnableClicked);
		Connect(Widget->GetUI()->SBRFCenter, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnRFCenterFreqChanged);
		Connect(Widget->GetUI()->SBRFSpan, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnRFFreqSpanChanged);
		Connect(Widget->GetUI()->SBRFFreqSpacing, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnRFFreqSpacingChanged);
		Connect(Widget->GetUI()->SBRFDwellTime, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnRFDwellTimeChanged);

		Connect(Widget->GetUI()->RBRFModulationTypeNone, &QRadioButton::clicked, this, &ODMR::OnRFModNoneClicked);
		Connect(Widget->GetUI()->RBRFModulationTypeSine, &QRadioButton::clicked, this, &ODMR::OnRFModSineClicked);
		Connect(Widget->GetUI()->RBRFModulationTypePulse, &QRadioButton::clicked, this, &ODMR::OnRFModPulseClicked);
		Connect(Widget->GetUI()->SBRFModulationFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnRFModFreqChanged);
		Connect(Widget->GetUI()->SBRFModulationDepth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnRFModDepthChanged);

		Connect(Widget->GetUI()->SBDataAcquisitionODMRSamplingRate, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnODMRSamplingRateChanged);

		Connect(Widget->GetUI()->LESaveDataPath, &QLineEdit::textChanged, this, &ODMR::OnSavePathChanged);
		Connect(Widget->GetUI()->SBSaveDataCurrentIndex, QOverload<int>::of(&QSpinBox::valueChanged), this, &ODMR::OnSaveIndexChanged);
		Connect(Widget->GetUI()->CBSaveDataEnable, &QCheckBox::checkStateChanged, this, &ODMR::OnAutosaveClicked);

		Connect(Widget->GetUI()->CBSensitivityEnable, &QCheckBox::checkStateChanged, this, &ODMR::OnRecordSensitivityClicked);
		Connect(Widget->GetUI()->CBSensitivityOncePerSweep, &QCheckBox::checkStateChanged, this, &ODMR::OnRecordSensitivityOncePerSweepClicked);
		Connect(Widget->GetUI()->CBSensitivityOffResEnable, &QCheckBox::checkStateChanged, this, &ODMR::OnRecordSensitivityOffResonanceClicked);
		Connect(Widget->GetUI()->SBSensitivityFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnSensitivityResonanceFreqChanged);
		Connect(Widget->GetUI()->SBSensitivityOffResFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnSensitivityOffResonanceFreqChanged);
		Connect(Widget->GetUI()->SBSensitivitySpan, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnSensitivityResonanceSpanChanged);
		Connect(Widget->GetUI()->SBSensitivitySamplingRate, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnSensitivitySamplingRateChanged);
		Connect(Widget->GetUI()->SBSensitivityDuration, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnSensitivityDurationChanged);

		Connect(Widget->GetUI()->GBSensitivityAnalysis, &QGroupBox::toggled, this, &ODMR::OnEnableSensitivityAnalysisClicked);
		Connect(Widget->GetUI()->SBGyromagneticRatio, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnGyromagneticRatioChanged);

		Connect(Widget->GetUI()->CBParamSweepEnable, &QCheckBox::checkStateChanged, this, &ODMR::OnEnableSweepSeriesClicked);
		Connect(Widget->GetUI()->CBParamSweepType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ODMR::OnSweepSeriesParamChanged);
		Connect(Widget->GetUI()->SBParamSweepStart, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnSweepSeriesStartChanged);
		Connect(Widget->GetUI()->SBParamSweepStop, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnSweepSeriesStopChanged);
		Connect(Widget->GetUI()->SBParamSweepStep, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ODMR::OnSweepSeriesStepChanged);
		Connect(Widget->GetUI()->CBParamSweepRetrace, &QCheckBox::checkStateChanged, this, &ODMR::OnSweepSeriesRetraceClicked);
		Connect(Widget->GetUI()->CBParamSweepAdvanceLastValue, &QCheckBox::checkStateChanged, this, &ODMR::OnSweepSeriesAdvanceLastValueClicked);

		Connect(Widget->GetUI()->BStart, &QPushButton::clicked, this, &ODMR::OnStartClicked);
		Connect(Widget->GetUI()->BStartSensitivity, &QPushButton::clicked, this, &ODMR::OnStartSensitivityClicked);
		Connect(Widget->GetUI()->BStop, &QPushButton::clicked, this, &ODMR::OnStopClicked);
		Connect(Widget->GetUI()->BRFOn, &QPushButton::clicked, this, &ODMR::OnRFOnClicked);
		Connect(Widget->GetUI()->BRFOff, &QPushButton::clicked, this, &ODMR::OnRFOffClicked);

		Connect(Widget->GetODMRGraph(), &DynExpQuick::DynExpLineGraphBackend::onSampleClicked, this, &ODMR::OnODMRGraphClicked);

		return Widget;
	}

	void ODMR::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<ODMRWidget>();

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(ModuleDataGetter());

			if (StateMachine.GetCurrentState()->GetState() == StateType::Initializing)
				return;

			Widget->InitializeUI(ModuleData);
			Widget->SetUIState(StateMachine.GetCurrentState(), ModuleData);
			Widget->UpdateUIData(ModuleData);
			Widget->UpdateODMRPlot(ModuleData->ODMRPlot);
			Widget->UpdateSensitivityPlot(ModuleData->SensitivityPlot);
		} // ModuleData unlocked here (heavy plot operations follow).
	}

	void ODMR::InitSweepValues(Util::SynchronizedPointer<ModuleDataType>& ModuleData)
	{
		NextRFPower = ModuleData->RFPower;
		NextRFModulationDepth = ModuleData->RFModulationDepth;
		NextAuxAnalogOutValue = (ModuleData->AuxAnalogOutMinValue <= 0 && ModuleData->AuxAnalogOutMaxValue >= 0) ? 0 : ModuleData->AuxAnalogOutMinValue;

		if (!ModuleData->SweepSeriesEnabled || !ModuleData->GetSweepNumberSteps())
			return;

		decltype(ModuleData->CurrentSweepIndex) SweepIndex = ModuleData->CurrentSweepIndex;
		if (ModuleData->SweepSeriesAdvanceLastValue && SweepIndex == 1)
			SweepIndex = ModuleData->GetSweepNumberSteps() - 1;
		else if (ModuleData->SweepSeriesAdvanceLastValue && SweepIndex > 1)
			--SweepIndex;

		switch (ModuleData->SweepSeries)
		{
		case ODMRData::SweepSeriesType::RFModulationDepth:
			NextRFModulationDepth = (ModuleData->SweepSeriesStart + SweepIndex * std::abs(ModuleData->SweepSeriesStep)) * 1e3;
			break;
		case ODMRData::SweepSeriesType::RFPower:
			NextRFPower = ModuleData->SweepSeriesStart + SweepIndex * std::abs(ModuleData->SweepSeriesStep);
			break;
		case ODMRData::SweepSeriesType::AnalogOut:
			NextAuxAnalogOutValue = ModuleData->SweepSeriesStart + SweepIndex * std::abs(ModuleData->SweepSeriesStep);
			break;
		}
	}

	void ODMR::InitRFGenerator(double Frequency, bool EnableRF, Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		ModuleData->GetRFGenerator()->SetSineFunction({ Frequency, NextRFPower, 0, 0 }, false, EnableRF);
		ModuleData->GetRFGenerator()->SetModulation({
				ModuleData->RFModulation == ODMRData::RFModulationType::None ?
					DynExpInstr::FunctionGeneratorDefs::ModulationDescType::ModulationType::Disabled :
					DynExpInstr::FunctionGeneratorDefs::ModulationDescType::ModulationType::Frequency,
				NextRFModulationDepth,
				ModuleData->RFModulationFreq,
				ModuleData->RFModulation == ODMRData::RFModulationType::Pulse ?
					DynExpInstr::FunctionGeneratorDefs::ModulationDescType::ModulationShapeType::Pulse :
				DynExpInstr::FunctionGeneratorDefs::ModulationDescType::ModulationShapeType::Sine
			});
	}

	void ODMR::SetAuxAnalogOutValue(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (ModuleData->TestFeature(ODMRData::FeatureType::AuxAnalogOut))
			ModuleData->GetAuxAnalogOut()->SetSync(NextAuxAnalogOutValue);
	}

	void ODMR::WaitUntilReadyAndTrigger(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		// Trigger on rising edge, so set to 0 now.
		ModuleData->GetTrigger()->SetSync(0);

		// Wait to ensure that trigger signal does not come too fast
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		// Prepare acquisition.
		ModuleData->GetSignalDetector()->Stop();
		ModuleData->GetSignalDetector()->Clear();
		ModuleData->GetSignalDetector()->Start();

		// Wait until every involved instrument is ready.
		DynExp::WaitForInstruments(*ModuleData->GetRFGenerator(), *ModuleData->GetSignalDetector(), *ModuleData->GetTrigger());

		// Trigger RF sweep and data acquisition now.
		ModuleData->GetTrigger()->SetSync(1);
	}

	void ODMR::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<ODMR>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->RFGenerator, ModuleData->GetRFGenerator());
		Instance->LockObject(ModuleParams->SignalDetector, ModuleData->GetSignalDetector());
		Instance->LockObject(ModuleParams->TriggerOut, ModuleData->GetTrigger());

		ModuleData->RFGeneratorMinFuncDesc = ModuleData->GetRFGenerator()->GetMinCaps();
		ModuleData->RFGeneratorMaxFuncDesc = ModuleData->GetRFGenerator()->GetMaxCaps();
		ModuleData->RFGeneratorDefaultFuncDesc = ModuleData->GetRFGenerator()->GetParamDefaults();

		if (dynamic_cast<const DynExpInstr::LockinAmplifier*>(ModuleData->GetSignalDetector().get()))
			ModuleData->SetFeature(ODMRData::FeatureType::LockinDetection);
		else if (dynamic_cast<const DynExpInstr::AnalogIn*>(ModuleData->GetSignalDetector().get()))
			ModuleData->SetFeature(ODMRData::FeatureType::AnalogInDetection);
		ModuleData->GetSignalDetector()->Stop();

		if (ModuleParams->AuxAnalogOut.ContainsID())
		{
			Instance->LockObject(ModuleParams->AuxAnalogOut, ModuleData->GetAuxAnalogOut());
			ModuleData->SetFeature(ODMRData::FeatureType::AuxAnalogOut);

			ModuleData->AuxAnalogOutValueUnit = ModuleData->GetAuxAnalogOut()->GetValueUnit();
			ModuleData->AuxAnalogOutMinValue = ModuleData->GetAuxAnalogOut()->GetUserMinValue();
			ModuleData->AuxAnalogOutMaxValue = ModuleData->GetAuxAnalogOut()->GetUserMaxValue();
		}
	}

	void ODMR::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->GetRFGenerator());
		Instance->UnlockObject(ModuleData->GetSignalDetector());
		Instance->UnlockObject(ModuleData->GetTrigger());
		Instance->UnlockObject(ModuleData->GetAuxAnalogOut());
	}

	void ODMR::OnRFPowerChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->RFPower = Value;
	}

	void ODMR::OnRFAutoEnableClicked(DynExp::ModuleInstance* Instance, Qt::CheckState Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->RFAutoEnabled = Checked == Qt::CheckState::Checked;
	}

	void ODMR::OnRFCenterFreqChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->RFCenterFreq = Value * 1e6;
	}

	void ODMR::OnRFFreqSpanChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->RFFreqSpan = Value * 1e6;
	}

	void ODMR::OnRFFreqSpacingChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->RFFreqSpacing = Value * 1e3;
	}

	void ODMR::OnRFDwellTimeChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->RFDwellTime = Value * 1e-3;
	}

	void ODMR::OnRFModNoneClicked(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());

		if (Checked)
			ModuleData->RFModulation = ODMRData::RFModulationType::None;
	}

	void ODMR::OnRFModSineClicked(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());

		if (Checked)
			ModuleData->RFModulation = ODMRData::RFModulationType::Sine;
	}

	void ODMR::OnRFModPulseClicked(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());

		if (Checked)
			ModuleData->RFModulation = ODMRData::RFModulationType::Pulse;
	}

	void ODMR::OnRFModFreqChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->RFModulationFreq = Value * 1e3;
	}

	void ODMR::OnRFModDepthChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->RFModulationDepth = Value * 1e3;
	}

	void ODMR::OnODMRSamplingRateChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->ODMRSamplingRate = Value;
	}

	void ODMR::OnSavePathChanged(DynExp::ModuleInstance* Instance, QString Path) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SaveDataPath = Path.toStdString();
	}

	void ODMR::OnSaveIndexChanged(DynExp::ModuleInstance* Instance, int Index) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->CurrentSaveIndex = Index;
	}

	void ODMR::OnAutosaveClicked(DynExp::ModuleInstance* Instance, Qt::CheckState Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->AutosaveEnabled = Checked == Qt::CheckState::Checked;
	}

	void ODMR::OnRecordSensitivityClicked(DynExp::ModuleInstance* Instance, Qt::CheckState Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SensitivityEnabled = Checked == Qt::CheckState::Checked;
	}

	void ODMR::OnRecordSensitivityOncePerSweepClicked(DynExp::ModuleInstance* Instance, Qt::CheckState Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SensitivityOncePerSweep = Checked == Qt::CheckState::Checked;
	}

	void ODMR::OnRecordSensitivityOffResonanceClicked(DynExp::ModuleInstance* Instance, Qt::CheckState Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SensitivityOffResonanceEnabled = Checked == Qt::CheckState::Checked;
	}

	void ODMR::OnSensitivityResonanceFreqChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SensitivityResonanceFreq = Value * 1e6;
	}

	void ODMR::OnSensitivityOffResonanceFreqChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SensitivityOffResonanceFreq = Value * 1e6;
	}

	void ODMR::OnSensitivityResonanceSpanChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SensitivityResonanceSpan = Value * 1e6;
	}

	void ODMR::OnSensitivitySamplingRateChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SensitivitySamplingRate = Value;
	}

	void ODMR::OnSensitivityDurationChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SensitivityDuration = Value;
	}

	void ODMR::OnEnableSensitivityAnalysisClicked(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SensitivityAnalysisEnabled = Checked;
	}

	void ODMR::OnGyromagneticRatioChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->GyromagneticRatio = Value * 1e6;
	}

	void ODMR::OnEnableSweepSeriesClicked(DynExp::ModuleInstance* Instance, Qt::CheckState Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SweepSeriesEnabled = Checked == Qt::CheckState::Checked;
	}

	void ODMR::OnSweepSeriesParamChanged(DynExp::ModuleInstance* Instance, int Index) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());

		switch (Index)
		{
		case 0: ModuleData->SweepSeries = ODMRData::SweepSeriesType::RFModulationDepth; break;
		case 1: ModuleData->SweepSeries = ODMRData::SweepSeriesType::RFPower; break;
		case 2:
			if (ModuleData->TestFeature(ODMRData::FeatureType::AuxAnalogOut))
				ModuleData->SweepSeries = ODMRData::SweepSeriesType::AnalogOut;
			break;
		}
	}

	void ODMR::OnSweepSeriesStartChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SweepSeriesStart = Value;
	}

	void ODMR::OnSweepSeriesStopChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SweepSeriesStop = Value;
	}

	void ODMR::OnSweepSeriesStepChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SweepSeriesStep = Value;
	}

	void ODMR::OnSweepSeriesRetraceClicked(DynExp::ModuleInstance* Instance, Qt::CheckState Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SweepSeriesRetrace = Checked == Qt::CheckState::Checked;
	}

	void ODMR::OnSweepSeriesAdvanceLastValueClicked(DynExp::ModuleInstance* Instance, Qt::CheckState Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->SweepSeriesAdvanceLastValue = Checked == Qt::CheckState::Checked;
	}

	void ODMR::OnStartClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		if (!IsReadyState())
			return;

		StateMachine.SetCurrentState(StateType::MeasurementSeriesInit);

		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->MeasurementMode = ODMRData::MeasurementModeType::All;
	}

	void ODMR::OnStartSensitivityClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		if (!IsReadyState())
			return;
			
		StateMachine.SetCurrentState(StateType::MeasurementSeriesInit);

		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->MeasurementMode = ODMRData::MeasurementModeType::SensitivityOnly;
	}

	void ODMR::OnStopClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());

		if (ModuleData->RFAutoEnabled)
			ModuleData->GetRFGenerator()->Stop();
		ModuleData->GetSignalDetector()->Stop();

		StateMachine.ResetContext();
		StateMachine.SetCurrentState(StateType::Ready);
	}

	void ODMR::OnRFOnClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->GetRFGenerator()->Start();
	}

	void ODMR::OnRFOffClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->GetRFGenerator()->Stop();
	}

	void ODMR::OnODMRGraphClicked(DynExp::ModuleInstance* Instance, QPointF Point) const
	{
		if (Point.isNull())
			return;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance->ModuleDataGetter());
		ModuleData->GetRFGenerator()->SetSineFunction({ Point.x() * 1e9 , ModuleData->RFPower, 0, 0});
	}

	StateType ODMR::ODMR::InitializingStateFunc(DynExp::ModuleInstance& Instance)
	{
		return StateType::Ready;
	}

	StateType ODMR::ODMR::ReadyStateFunc(DynExp::ModuleInstance& Instance)
	{
		return StateType::Ready;
	}

	StateType ODMR::MeasurementSeriesInitFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

		ModuleData->CurrentSweepIndex = 0;
		InitSweepValues(ModuleData);
		
		if (ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection))
		{
			auto LockinAmplifier = ModuleData->GetLockinAmplifier();
			LockinAmplifier->SetTriggerMode(DynExpInstr::LockinAmplifierDefs::TriggerModeType::ExternSingle);
			LockinAmplifier->SetTriggerEdge(DynExpInstr::LockinAmplifierDefs::TriggerEdgeType::Rise);
			LockinAmplifier->SetEnable(true);
		}

		return ModuleData->MeasurementMode == ODMRData::MeasurementModeType::All ? StateType::ODMRTraceInit : StateType::SensitivityInit;
	}

	StateType ODMR::MeasurementSeriesStepFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

		if (!ModuleData->SweepSeriesEnabled || ++ModuleData->CurrentSweepIndex >= ModuleData->GetSweepNumberSteps())
		{
			++ModuleData->CurrentSaveIndex;

			if (ModuleData->SweepSeriesEnabled && ModuleData->SweepSeriesRetrace)
			{
				ModuleData->CurrentSweepIndex = 0;
				InitSweepValues(ModuleData);

				SetAuxAnalogOutValue(ModuleData);
				InitRFGenerator(ModuleData->GetRFStartFreq(), false, ModuleData);
			}
			if (ModuleData->RFAutoEnabled)
				ModuleData->GetRFGenerator()->Stop();

			return StateType::Ready;
		}

		InitSweepValues(ModuleData);

		return ModuleData->MeasurementMode == ODMRData::MeasurementModeType::All ? StateType::ODMRTraceInit : StateType::SensitivityInit;
	}

	StateType ODMR::ODMRTraceInitFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

		SetAuxAnalogOutValue(ModuleData);
		InitRFGenerator(ModuleData->GetRFStartFreq(), ModuleData->RFAutoEnabled, ModuleData);
		ModuleData->GetRFGenerator()->SetSweep({
			DynExpInstr::FunctionGeneratorDefs::SweepDescType::SweepType::Frequency,
			ModuleData->GetRFStartFreq(),
			ModuleData->GetRFStopFreq(),
			ModuleData->RFFreqSpacing,
			ModuleData->RFDwellTime * 1000,
			true
		});
		ModuleData->GetRFGenerator()->SetTrigger({
			DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternSingle,
			DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType::Rise
		});

		if (ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection))
		{
			auto LockinAmplifier = ModuleData->GetLockinAmplifier();
			LockinAmplifier->SetStreamSize(ModuleData->RFDwellTime * ModuleData->GetNumSamples() * ModuleData->ODMRSamplingRate);
			LockinAmplifier->SetSamplingRate(ModuleData->ODMRSamplingRate);
		}

		WaitUntilReadyAndTrigger(ModuleData);

		return StateType::ODMRTraceWait;
	}

	StateType ODMR::ODMRTraceWaitFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

		return ModuleData->GetSignalDetector()->HasFinished() ? StateType::ODMRTraceFinish : StateType::ODMRTraceWait;
	}

	StateType ODMR::ODMRTraceFinishFunc(DynExp::ModuleInstance& Instance)
	{
		std::vector<std::tuple<double, double, double>> SaveSamples;
		bool Save = false;
		bool PerformSensitivityMeasurement = false;
		double RFStartFreq{};
		decltype(ODMRData::RFDwellTime) RFDwellTime{};
		decltype(ODMRData::RFFreqSpacing) RFFreqSpacing{};
		bool IsBasicSampleTimeUsed{};
		double SamplingRate{ .0 };
		DynExpInstr::DataStreamBase::BasicSampleListType ODMRSamples;
		decltype(ODMRPlotType::DataPoints) ODMRDataPoints, ProcessedODMRDataPoints;
		decltype(ODMRData::SensitivityResonanceFreq) SensitivityResonanceFreq{};
		decltype(ODMRData::SensitivityResonanceSpan) SensitivityResonanceSpan{};
		decltype(ODMRPlotType::FitParams) ODMRFitParams{};
		decltype(ODMRPlotType::FitPoints) ODMRFitPoints, ProcessedODMRFitPoints;
		decltype(ODMRPlotType::PlotInfo) ODMRPlotInfo;
		std::vector<double> FitXData;
		std::vector<double> FitYData;
		std::string Filename;
		std::string ValueUnitStr;
		std::stringstream CSVData;

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

			if (!ModuleData->GetSignalDetector()->CanRead())
				return StateType::ODMRTraceFinish;

			Save = ModuleData->AutosaveEnabled;
			PerformSensitivityMeasurement = ModuleData->SensitivityEnabled && (!ModuleData->SensitivityOncePerSweep || !ModuleData->CurrentSweepIndex);
			RFStartFreq = ModuleData->GetRFStartFreq();
			RFDwellTime = ModuleData->RFDwellTime;
			RFFreqSpacing = ModuleData->RFFreqSpacing;
			SensitivityResonanceFreq = ModuleData->SensitivityResonanceFreq;
			SensitivityResonanceSpan = ModuleData->SensitivityResonanceSpan;
			ODMRPlotInfo = ModuleData->ODMRPlot.PlotInfo;

			auto SignalDetectorData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(ModuleData->GetSignalDetector()->GetInstrumentData());
			SignalDetectorData->GetSampleStream()->SeekBeg(std::ios_base::in);
			ODMRSamples = SignalDetectorData->GetSampleStream()->ReadBasicSamples(SignalDetectorData->GetSampleStream()->GetStreamSizeRead());

			IsBasicSampleTimeUsed = SignalDetectorData->GetSampleStream()->IsBasicSampleTimeUsed();
			if (!IsBasicSampleTimeUsed && ModuleData->TestFeature(ODMRData::FeatureType::AnalogInDetection))
			{
				SamplingRate = ModuleData->GetAnalogIn()->GetNumericSampleStreamParams().SamplingRate;

				if (SamplingRate == .0)
					throw Util::InvalidDataException("The analog input channel's sampling rate must not be 0.");
			}

		} // ModuleData unlocked here (heavy calculation follows).

		for (size_t i = 0; i < ODMRSamples.size(); ++i)
		{
			const auto& Sample = ODMRSamples[i];

			auto Frequency = RFStartFreq + (IsBasicSampleTimeUsed ? Sample.Time : (static_cast<double>(i) / SamplingRate)) / RFDwellTime * RFFreqSpacing;
			ODMRDataPoints.push_back({ Frequency / 1e9, Sample.Value });
			if (Save)
				SaveSamples.emplace_back(std::make_tuple(Frequency, Sample.Time, Sample.Value));

			if (PerformSensitivityMeasurement &&
				Frequency >= SensitivityResonanceFreq - SensitivityResonanceSpan / 2 &&
				Frequency <= SensitivityResonanceFreq + SensitivityResonanceSpan / 2)
			{
				FitXData.push_back(Frequency - SensitivityResonanceFreq);
				FitYData.push_back(Sample.Value);
			}
		}

		if (PerformSensitivityMeasurement)
		{
			double c0{0}, c1{0}, cov00{0}, cov01{0}, cov11{0}, chisq{0};
			gsl_fit_linear(FitXData.data(), 1, FitYData.data(), 1, FitXData.size(), &c0, &c1, &cov00, &cov01, &cov11, &chisq);
			ODMRFitParams = { c0, c1 };

			for (const auto& f : FitXData)
				ODMRFitPoints.push_back({ (f + SensitivityResonanceFreq) / 1e9, c0 + c1 * f });
		}

		ODMRPlotInfo.Reset();
		ODMRPlotInfo.ProcessSamples(ODMRDataPoints, ProcessedODMRDataPoints, 0);
		ODMRPlotInfo.ProcessSamples(ODMRFitPoints, ProcessedODMRFitPoints, 1);
		ODMRPlotInfo.AdjustAxesLimits();

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

			ModuleData->ODMRPlot.DataPoints = std::move(ProcessedODMRDataPoints);
			ModuleData->ODMRPlot.FitPoints = std::move(ProcessedODMRFitPoints);
			ModuleData->ODMRPlot.FitParams = ODMRFitParams;
			ModuleData->ODMRPlot.HasChanged = true;
			ModuleData->ODMRPlot.PlotInfo = ODMRPlotInfo;

			Filename = Util::RemoveExtFromPath(ModuleData->SaveDataPath) + "_ODMR" + Util::ToStr(ModuleData->CurrentSaveIndex) + "_Sweep" + Util::ToStr(ModuleData->CurrentSweepIndex) + ".csv";
			ValueUnitStr = ModuleData->GetSignalDetector()->GetValueUnitStr();

			if (Save)
				CSVData = ModuleData->AssembleCSVHeader(NextRFPower, NextRFModulationDepth, NextAuxAnalogOutValue, false);
		} // ModuleData unlocked here.

		if (Save)
		{
			CSVData << "f(Hz);t(s);Value(" << ValueUnitStr << ")\n";
			for (const auto& Sample : SaveSamples)
				CSVData << std::get<0>(Sample) << ";" << std::get<1>(Sample) << ";" << std::get<2>(Sample) << "\n";

			Util::SaveToFile(QString::fromStdString(Filename), CSVData.str());
		}

		return PerformSensitivityMeasurement ? StateType::SensitivityInit : StateType::MeasurementSeriesStep;
	}

	StateType ODMR::SensitivityInitFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

		// No sensitivity calculation if detection is not performed with a lock-in amplifer since in this case it is not possible
		// to measure the ODMR signal's derivative directly.
		if (!ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection))
		{
			StateMachine.ResetContext();
			return StateType::MeasurementSeriesStep;
		}

		SetAuxAnalogOutValue(ModuleData);
		ModuleData->GetRFGenerator()->SetSweep({});		// Disable sweep.
		ModuleData->GetRFGenerator()->SetTrigger({});	// Disable trigger.
		auto RFFreq = StateMachine.GetContext() == &SensitivityOffResonanceContext ? ModuleData->SensitivityOffResonanceFreq : ModuleData->SensitivityResonanceFreq;
		InitRFGenerator(RFFreq, ModuleData->RFAutoEnabled, ModuleData);

		auto LockinAmplifier = ModuleData->GetLockinAmplifier();
		LockinAmplifier->SetStreamSize(ModuleData->SensitivityDuration * ModuleData->SensitivitySamplingRate);
		LockinAmplifier->SetSamplingRate(ModuleData->SensitivitySamplingRate);

		WaitUntilReadyAndTrigger(ModuleData);

		return StateType::SensitivityWait;
	}

	StateType ODMR::SensitivityWaitFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

		return ModuleData->GetSignalDetector()->HasFinished() ? StateType::SensitivityFinish : StateType::SensitivityWait;
	}

	StateType ODMR::SensitivityFinishFunc(DynExp::ModuleInstance& Instance)
	{
		std::vector<std::tuple<double, double>> SaveSamples;
		ODMRData::MeasurementModeType MeasurementMode = ODMRData::MeasurementModeType::All;
		bool Save = false;
		bool SensitivityOffResonanceEnabled = false;
		bool SensitivityAnalysisEnabled = false;
		decltype(ODMRPlotType::FitParams) ODMRFitParams{};
		decltype(ODMRData::GyromagneticRatio) GyromagneticRatio{};
		DynExpInstr::DataStreamBase::BasicSampleListType SensitivitySamples;
		std::vector<std::complex<double>> SensitivityASD;
		decltype(SensitivityPlotType::DataPoints) SensitivityDataPoints, ProcessedSensitivityDataPoints;
		decltype(SensitivityPlotType::PlotInfo) SensitivityPlotInfo;
		std::string Filename;
		std::string FilenamePrefix;
		std::string ValueUnitStr;
		std::stringstream CSVData;

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

			if (!ModuleData->GetSignalDetector()->CanRead())
				return StateType::SensitivityFinish;

			MeasurementMode = ModuleData->MeasurementMode;
			Save = ModuleData->AutosaveEnabled;
			SensitivityOffResonanceEnabled = ModuleData->SensitivityOffResonanceEnabled;
			SensitivityAnalysisEnabled = ModuleData->SensitivityAnalysisEnabled;
			ODMRFitParams = ModuleData->ODMRPlot.FitParams;
			GyromagneticRatio = ModuleData->GyromagneticRatio;
			SensitivityPlotInfo = ModuleData->SensitivityPlot.PlotInfo;

			auto SignalDetectorData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(ModuleData->GetSignalDetector()->GetInstrumentData());
			SignalDetectorData->GetSampleStream()->SeekBeg(std::ios_base::in);
			SensitivitySamples = SignalDetectorData->GetSampleStream()->ReadBasicSamples(SignalDetectorData->GetSampleStream()->GetStreamSizeRead());
		} // ModuleData unlocked here (heavy calculation follows).

		for (const auto& Sample : SensitivitySamples)
		{
			SaveSamples.emplace_back(std::make_tuple(Sample.Time, Sample.Value));

			if (MeasurementMode == ODMRData::MeasurementModeType::All && StateMachine.GetContext() != &SensitivityOffResonanceContext)
				SensitivityASD.emplace_back(Sample.Value / std::get<1>(ODMRFitParams) / GyromagneticRatio, 0);
		}

		if (SensitivityASD.size() > 2 && SensitivityAnalysisEnabled && StateMachine.GetContext() != &SensitivityOffResonanceContext)
		{
			const auto Length = static_cast<double>(SensitivityASD.size());

			SensitivityASD = Util::FFT(SensitivityASD);
			std::transform(SensitivityASD.cbegin(), SensitivityASD.cend(), SensitivityASD.begin(), [Length](auto x) { return std::abs(x / Length); });

			// Make single-sided spectrum
			SensitivityASD.erase(SensitivityASD.cbegin() + SensitivityASD.size() / 2 + 1, SensitivityASD.cend());
			std::transform(SensitivityASD.cbegin() + 1, SensitivityASD.cend(), SensitivityASD.begin() + 1, [Length](auto x) { return 2.0 * x; });
				
			// Assume SensitivitySamples sorted by Time. Assume samples equally spaced in time.
			double TimeSamplingRate = 1 / (std::abs(SensitivitySamples.back().Time - SensitivitySamples.front().Time) / (SensitivitySamples.size() - 1));
			std::vector<double> Frequencies(Length / 2 + 1, 0);
			std::iota(Frequencies.begin(), Frequencies.end(), 0);
			std::transform(Frequencies.cbegin(), Frequencies.cend(), Frequencies.begin(), [TimeSamplingRate, Length](auto x) { return TimeSamplingRate * x / Length; });

			double FrequencySamplingRate = std::abs(Frequencies.back() - Frequencies.front()) / (Frequencies.size() - 1);
			std::transform(SensitivityASD.cbegin(), SensitivityASD.cend(), SensitivityASD.begin(), [FrequencySamplingRate](auto x) { return x / std::sqrt(FrequencySamplingRate); });

			if (SensitivityASD.size() == Frequencies.size())
				for (size_t i = 1; i < SensitivityASD.size(); ++i)		// Ignore 0 Hz frequency component
					SensitivityDataPoints.push_back({ Frequencies[i], SensitivityASD[i].real() });
		}

		SensitivityPlotInfo.Reset();
		SensitivityPlotInfo.ProcessSamples(SensitivityDataPoints, ProcessedSensitivityDataPoints, 0);
		SensitivityPlotInfo.AdjustAxesLimits();

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ODMR>(Instance.ModuleDataGetter());

			if (StateMachine.GetContext() != &SensitivityOffResonanceContext)
			{
				ModuleData->SensitivityPlot.DataPoints = std::move(ProcessedSensitivityDataPoints);
				ModuleData->SensitivityPlot.HasChanged = true;
				ModuleData->SensitivityPlot.PlotInfo = SensitivityPlotInfo;

				FilenamePrefix = "Sensitivity";
			}
			else
				FilenamePrefix = "OffResSensitivity";

			Filename = Util::RemoveExtFromPath(ModuleData->SaveDataPath) + "_" + FilenamePrefix + Util::ToStr(ModuleData->CurrentSaveIndex) + "_Sweep" + Util::ToStr(ModuleData->CurrentSweepIndex) + ".csv";
			ValueUnitStr = ModuleData->GetSignalDetector()->GetValueUnitStr();

			if (Save)
				CSVData = ModuleData->AssembleCSVHeader(NextRFPower, NextRFModulationDepth, NextAuxAnalogOutValue, StateMachine.GetContext() == &SensitivityOffResonanceContext);
		} // ModuleData unlocked here.

		if (Save)
		{
			CSVData << "t(s);Value(" << ValueUnitStr << ")\n";
			CSVData << std::setprecision(12);
			for (const auto& Sample : SaveSamples)
				CSVData << std::get<0>(Sample) << ";" << std::get<1>(Sample) << "\n";

			Util::SaveToFile(QString::fromStdString(Filename), CSVData.str());
		}

		if (SensitivityOffResonanceEnabled)
		{
			if (StateMachine.GetContext() == &SensitivityOffResonanceContext)
				StateMachine.ResetContext();
			else
			{
				StateMachine.SetContext(&SensitivityOffResonanceContext);
				return StateType::SensitivityInit;
			}
		}

		return StateType::MeasurementSeriesStep;
	}
}