// This file is part of DynExp.

/**
 * @file ODMR.h
 * @brief Implementation of a module to perform optically detected magnetic resonance (ODMR)
 * measurements.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../../MetaInstruments/FunctionGenerator.h"
#include "../../MetaInstruments/LockinAmplifier.h"
#include "../../MetaInstruments/AnalogIn.h"
#include "../../MetaInstruments/AnalogOut.h"
#include "../../MetaInstruments/DigitalOut.h"

#include "ODMRWidget.h"

namespace DynExpModule::ODMR
{
	class ODMRData : public DynExp::QModuleDataBase
	{
	public:
		enum class FeatureType { LockinDetection, AnalogInDetection, AuxAnalogOut, NUM_ELEMENTS };
		enum class MeasurementModeType { All, SensitivityOnly };
		enum class RFModulationType { None, Sine, Pulse };
		enum class SweepSeriesType { RFModulationDepth, RFPower, AnalogOut };

		ODMRData() { Init(); }
		virtual ~ODMRData() = default;

		template <size_t N>
		bool TestFeature(const std::array<FeatureType, N>& Flags) const { return Features.Test(Flags); }

		bool TestFeature(FeatureType Flag) { return Features.Test(Flag); }
		void SetFeature(FeatureType Flag) { Features.Set(Flag); }

		auto& GetRFGenerator() noexcept { return RFGenerator; }
		auto& GetSignalDetector() noexcept { return SignalDetector; }
		auto GetLockinAmplifier() { return DynExp::dynamic_Object_cast<DynExpInstr::LockinAmplifier>(GetSignalDetector().get()); }
		auto GetAnalogIn() { return DynExp::dynamic_Object_cast<DynExpInstr::AnalogIn>(GetSignalDetector().get()); }
		auto& GetTrigger() noexcept { return TriggerOut; }
		auto& GetAuxAnalogOut() noexcept { return AuxAnalogOut; }

		double GetRFStartFreq() const noexcept { return std::abs(RFCenterFreq) - std::abs(RFFreqSpan) / 2; }
		double GetRFStopFreq() const noexcept { return std::abs(RFCenterFreq) + std::abs(RFFreqSpan) / 2; }
		unsigned long long GetNumSamples() const noexcept { return std::abs(RFFreqSpan / RFFreqSpacing); }
		unsigned long long GetSweepNumberSteps() const noexcept;
		std::stringstream AssembleCSVHeader(double RFPower, double RFModulationDepth, double AuxAnalogOutValue, bool IsRFOffResonance);

		double RFPower{};						// in units of RFGenerator
		bool RFAutoEnabled{};
		double RFCenterFreq{};					// in Hz
		double RFFreqSpan{};					// in Hz
		double RFFreqSpacing{};					// in Hz
		double RFDwellTime{};					// in s

		RFModulationType RFModulation{};
		double RFModulationFreq{};				// in Hz
		double RFModulationDepth{};				// in Hz

		double ODMRSamplingRate{};				// in samples/s

		MeasurementModeType MeasurementMode{};
		std::string SaveDataPath;
		unsigned int CurrentSaveIndex{};
		bool AutosaveEnabled{};

		bool SensitivityEnabled{};
		bool SensitivityOncePerSweep{};
		bool SensitivityOffResonanceEnabled{};
		double SensitivityResonanceFreq{};		// in Hz
		double SensitivityOffResonanceFreq{};	// in Hz
		double SensitivityResonanceSpan{};		// in Hz
		double SensitivitySamplingRate{};		// in samples/s
		double SensitivityDuration{};			// in s

		bool SensitivityAnalysisEnabled{};
		double GyromagneticRatio{};				// in Hz/T

		bool SweepSeriesEnabled{};
		SweepSeriesType SweepSeries{};
		double SweepSeriesStart{};				// in kHz or units of RFGenerator
		double SweepSeriesStop{};				// in kHz or units of RFGenerator
		double SweepSeriesStep{};				// in kHz or units of RFGenerator
		bool SweepSeriesRetrace{};
		bool SweepSeriesAdvanceLastValue{};
		unsigned long long CurrentSweepIndex;

		DynExpInstr::FunctionGeneratorDefs::FunctionDescType RFGeneratorMinFuncDesc;
		DynExpInstr::FunctionGeneratorDefs::FunctionDescType RFGeneratorMaxFuncDesc;
		DynExpInstr::FunctionGeneratorDefs::FunctionDescType RFGeneratorDefaultFuncDesc;

		DynExpInstr::DataStreamInstrumentData::UnitType AuxAnalogOutValueUnit;
		DynExpInstr::DataStreamInstrumentData::ValueType AuxAnalogOutMinValue;
		DynExpInstr::DataStreamInstrumentData::ValueType AuxAnalogOutMaxValue;

		double AcquisitionTime{};				// in s

		ODMRPlotType ODMRPlot;
		SensitivityPlotType SensitivityPlot;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<ODMRData>) {};

		void Init();

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::FunctionGenerator> RFGenerator;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::DataStreamInstrument> SignalDetector;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::DigitalOut> TriggerOut;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::AnalogOut> AuxAnalogOut;

		Util::FeatureTester<FeatureType> Features;
	};

	class ODMRParams : public DynExp::QModuleParamsBase
	{
	public:
		ODMRParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~ODMRParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "ODMRParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::FunctionGenerator>> RFGenerator = { *this, GetCore().GetInstrumentManager(),
			"RFGenerator", "RF generator", "RF generator to drive spin transitions", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>> SignalDetector = { *this, GetCore().GetInstrumentManager(),
			"SignalDetector", "Signal detector", "Detector to record the ODMR signal (might be a lock-in amplifier or any DAQ device)", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::DigitalOut>> TriggerOut = { *this, GetCore().GetInstrumentManager(),
			"TriggerOut", "Trigger output (DO)", "Trigger output to synchronize an RF frequency sweep and the data acquisition", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::AnalogOut>> AuxAnalogOut = { *this, GetCore().GetInstrumentManager(),
			"AuxAnalogOut", "Auxiliary analog output (AO)", "Auxiliary output to perform ODMR parameter sweeps with", DynExpUI::Icons::Instrument, true };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class ODMRConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = ODMR;
		using ParamsType = ODMRParams;

		ODMRConfigurator() = default;
		virtual ~ODMRConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<ODMRConfigurator>(ID, Core); }
	};

	class ODMR : public DynExp::QModuleBase
	{
	public:
		using ParamsType = ODMRParams;
		using ConfigType = ODMRConfigurator;
		using ModuleDataType = ODMRData;

		constexpr static auto Name() noexcept { return "ODMR"; }
		constexpr static auto Category() noexcept { return "Experiments"; }

		ODMR(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~ODMR() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final;
		bool TreatModuleExceptionsAsWarnings() const override final { return StateMachine.GetCurrentState()->GetState() != StateType::Initializing; }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Helper functions
		bool IsReadyState() const noexcept { return StateMachine.GetCurrentState()->GetState() == StateType::Ready; }
		void InitSweepValues(Util::SynchronizedPointer<ModuleDataType>& ModuleData);
		void InitRFGenerator(double Frequency, bool EnableRF, Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void SetAuxAnalogOutValue(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void WaitUntilReadyAndTrigger(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void ConnectChartWidgets(QLineSeries* ODMRLineSeries);

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		
		void OnRFPowerChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnRFAutoEnableClicked(DynExp::ModuleInstance* Instance, int Checked) const;
		void OnRFCenterFreqChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnRFFreqSpanChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnRFFreqSpacingChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnRFDwellTimeChanged(DynExp::ModuleInstance* Instance, double Value) const;

		void OnRFModNoneClicked(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnRFModSineClicked(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnRFModPulseClicked(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnRFModFreqChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnRFModDepthChanged(DynExp::ModuleInstance* Instance, double Value) const;

		void OnODMRSamplingRateChanged(DynExp::ModuleInstance* Instance, double Value) const;

		void OnSavePathChanged(DynExp::ModuleInstance* Instance, QString Path) const;
		void OnSaveIndexChanged(DynExp::ModuleInstance* Instance, int Index) const;
		void OnAutosaveClicked(DynExp::ModuleInstance* Instance, int Checked) const;

		void OnRecordSensitivityClicked(DynExp::ModuleInstance* Instance, int Checked) const;
		void OnRecordSensitivityOncePerSweepClicked(DynExp::ModuleInstance* Instance, int Checked) const;
		void OnRecordSensitivityOffResonanceClicked(DynExp::ModuleInstance* Instance, int Checked) const;
		void OnSensitivityResonanceFreqChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnSensitivityOffResonanceFreqChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnSensitivityResonanceSpanChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnSensitivitySamplingRateChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnSensitivityDurationChanged(DynExp::ModuleInstance* Instance, double Value) const;

		void OnEnableSensitivityAnalysisClicked(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnGyromagneticRatioChanged(DynExp::ModuleInstance* Instance, double Value) const;

		void OnEnableSweepSeriesClicked(DynExp::ModuleInstance* Instance, int Checked) const;
		void OnSweepSeriesParamChanged(DynExp::ModuleInstance* Instance, int Index) const;
		void OnSweepSeriesStartChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnSweepSeriesStopChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnSweepSeriesStepChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnSweepSeriesRetraceClicked(DynExp::ModuleInstance* Instance, int Checked) const;
		void OnSweepSeriesAdvanceLastValueClicked(DynExp::ModuleInstance* Instance, int Checked) const;

		void OnStartClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnStartSensitivityClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnStopClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnRFOnClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnRFOffClicked(DynExp::ModuleInstance* Instance, bool) const;

		void OnODMRChartHovered(DynExp::ModuleInstance* Instance, QPointF Point, bool State) const;
		void OnODMRChartClicked(DynExp::ModuleInstance* Instance, QPointF Point) const;

		// State functions for state machine
		StateType InitializingStateFunc(DynExp::ModuleInstance& Instance);
		StateType ReadyStateFunc(DynExp::ModuleInstance& Instance);
		StateType MeasurementSeriesInitFunc(DynExp::ModuleInstance& Instance);
		StateType MeasurementSeriesStepFunc(DynExp::ModuleInstance& Instance);
		StateType ODMRTraceInitFunc(DynExp::ModuleInstance& Instance);
		StateType ODMRTraceWaitFunc(DynExp::ModuleInstance& Instance);
		StateType ODMRTraceFinishFunc(DynExp::ModuleInstance& Instance);
		StateType SensitivityInitFunc(DynExp::ModuleInstance& Instance);
		StateType SensitivityWaitFunc(DynExp::ModuleInstance& Instance);
		StateType SensitivityFinishFunc(DynExp::ModuleInstance& Instance);

		// States for state machine
		static constexpr auto InitializingState = Util::StateMachineState(StateType::Initializing,
			&ODMR::InitializingStateFunc, "Initializing module...");
		static constexpr auto ReadyState = Util::StateMachineState(StateType::Ready,
			&ODMR::ReadyStateFunc, "Ready");
		static constexpr auto MeasurementSeriesInitState = Util::StateMachineState(StateType::MeasurementSeriesInit,
			&ODMR::MeasurementSeriesInitFunc, "Measuring...");
		static constexpr auto MeasurementSeriesStepState = Util::StateMachineState(StateType::MeasurementSeriesStep,
			&ODMR::MeasurementSeriesStepFunc, "Measuring...");
		static constexpr auto ODMRTraceInitState = Util::StateMachineState(StateType::ODMRTraceInit,
			&ODMR::ODMRTraceInitFunc, "Acquiring ODMR trace...");
		static constexpr auto ODMRTraceWaitState = Util::StateMachineState(StateType::ODMRTraceWait,
			&ODMR::ODMRTraceWaitFunc, "Acquiring ODMR trace...");
		static constexpr auto ODMRTraceFinishState = Util::StateMachineState(StateType::ODMRTraceFinish,
			&ODMR::ODMRTraceFinishFunc, "Processing and saving ODMR trace...");
		static constexpr auto SensitivityInitState = Util::StateMachineState(StateType::SensitivityInit,
			&ODMR::SensitivityInitFunc, "Acquiring sensitivity series...");
		static constexpr auto SensitivityWaitState = Util::StateMachineState(StateType::SensitivityWait,
			&ODMR::SensitivityWaitFunc, "Acquiring sensitivity series...");
		static constexpr auto SensitivityFinishState = Util::StateMachineState(StateType::SensitivityFinish,
			&ODMR::SensitivityFinishFunc, "Processing and saving sensitivity series...");

		// Contexts for state machine
		const Util::StateMachineContext<StateMachineStateType> SensitivityOffResonanceContext = { {}, "Acquiring off-resonance sensitivity series..." };

		// Logical const-ness: allow events to set the state machine's current state.
		mutable Util::StateMachine<StateMachineStateType> StateMachine;

		double NextRFPower{};
		double NextRFModulationDepth{};
		double NextAuxAnalogOutValue{};

		size_t NumFailedUpdateAttempts = 0;
	};
}