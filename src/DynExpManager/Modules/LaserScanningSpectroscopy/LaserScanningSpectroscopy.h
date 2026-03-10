// This file is part of DynExp.

/**
 * @file LaserScanningSpectroscopy.h
 * @brief Implementation of a module to perform photoluminescence excitation spectroscopy.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../../MetaInstruments/Laser.h"
#include "../../Instruments/InterModuleCommunicator.h"

#include "CommonModuleEvents.h"

#include <QWidget>

namespace Ui
{
	class LaserScanningSpectroscopy;
}

namespace DynExpModule::LaserScanningSpectroscopy
{
	class LaserScanningSpectroscopy;
	class LaserScanningSpectroscopyData;

	enum class StateType {
		Ready,
		WaitForSettingFrequency,
		WaitForCapturing
	};

	using StateMachineStateType = Util::StateMachineState<StateType(LaserScanningSpectroscopy::*)(DynExp::ModuleInstance&)>;

	class LaserScanningSpectroscopyWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		LaserScanningSpectroscopyWidget(LaserScanningSpectroscopy& Owner, QModuleWidget* parent = nullptr);
		~LaserScanningSpectroscopyWidget() = default;

		bool AllowResize() const noexcept override final { return true; }
		const auto GetUI() const noexcept { return ui.get(); }
		bool GetUIInitialized() const noexcept { return UIInitialized; }

		void InitializeUI(Util::SynchronizedPointer<LaserScanningSpectroscopyData>& ModuleData);

	private:
		std::unique_ptr<Ui::LaserScanningSpectroscopy> ui;

		bool UIInitialized = false;

	private slots:
		void OnPathBrowseClicked();
	};

	class LaserScanningSpectroscopyData : public DynExp::QModuleDataBase
	{
	public:
		LaserScanningSpectroscopyData() { Init(); }
		virtual ~LaserScanningSpectroscopyData() = default;

		auto& GetPLECommunicator() { return PLECommunicator; }
		auto& GetWFCommunicator() { return WFCommunicator; }
		auto& GetLaser() { return Laser; }

		bool IsStepwiseScan;
		double LowerFrequencyLimit;
		double UpperFrequencyLimit;
		double FrequencyRange;
		double ModeHopFreeTuningRange;
		double CenterFrequency;
		double StepSize;
		int NumberOfSteps;
		int NumberOfRepetitions;
		double ScanStartFrequency;
		double ScanEndFrequency;
		bool ScanBackAndForth;
		int CurrentStepCount;
		int CurrentRepCount;
		std::filesystem::path FileSavePath;
		DynExpInstr::LaserData::LaserStateType LaserState;
		
		StateType LaserScanningSpectroscopyState;
		int LaserScanningSpectroscopyProgress;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<LaserScanningSpectroscopyData>) {};

		void Init();

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::Laser> Laser;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> PLECommunicator;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> WFCommunicator;
	};

	class LaserScanningSpectroscopyParams : public DynExp::QModuleParamsBase
	{
	public:
		LaserScanningSpectroscopyParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~LaserScanningSpectroscopyParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "LaserScanningSpectroscopyParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::Laser>> Laser = { *this, GetCore().GetInstrumentManager(),
			"Laser", "Laser", "Laser instrument to perform spectroscopy with by scanning its frequency", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> PLECommunicator = { *this, GetCore().GetInstrumentManager(),
			"PLEInterModuleCommunicator", "PLE inter-module communicator", "Inter-module communicator to control data acquisition", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> WFCommunicator = { *this, GetCore().GetInstrumentManager(),
			"WFInterModuleCommunicator", "WF inter-module communicator", "Inter-module communicator to communicate with widefield microscope module", DynExpUI::Icons::Instrument, true };
		Param<ParamsConfigDialog::NumberType> CapturingTimeDifference = { *this, "CapturingTimeDifference",
				"Capturing time difference (ms)", "Time difference between capturing events in continuous scan mode",
				false, 500, 1};

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class LaserScanningSpectroscopyConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = LaserScanningSpectroscopy;
		using ParamsType = LaserScanningSpectroscopyParams;

		LaserScanningSpectroscopyConfigurator() = default;
		virtual ~LaserScanningSpectroscopyConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<LaserScanningSpectroscopyConfigurator>(ID, Core); }
	};

	class LaserScanningSpectroscopy : public DynExp::QModuleBase
	{
	public:
		using ParamsType = LaserScanningSpectroscopyParams;
		using ConfigType = LaserScanningSpectroscopyConfigurator;
		using ModuleDataType = LaserScanningSpectroscopyData;

		constexpr static auto Name() noexcept { return "Laser Scanning Spectroscopy"; }
		constexpr static auto Category() noexcept { return "Experiments"; }

		LaserScanningSpectroscopy(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~LaserScanningSpectroscopy() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(50); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;
		
		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Helper functions
		bool IsReadyState() const noexcept;
		bool IsSettingFrequencyState() const noexcept;
		bool IsCapturingState() const noexcept;
		void FrequencyStep(DynExp::ModuleInstance* Instance) const;
		std::filesystem::path BuildFilename(Util::SynchronizedPointer<ModuleDataType>& ModuleData, std::string_view FilenameSuffix) const;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnStart(DynExp::ModuleInstance* Instance) const;
		void OnStartClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnStop(DynExp::ModuleInstance* Instance) const;
		void OnStopClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnLowerFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double LowerFrequencyLimit) const;
		void OnUpperFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double UpperFrequencyLimit) const;
		void OnFrequencyRangeChanged(DynExp::ModuleInstance* Instance, double FrequencyRange) const;
		void OnCenterFrequencyChanged(DynExp::ModuleInstance* Instance, double CenterFrequency) const;
		void OnStepSizeChanged(DynExp::ModuleInstance* Instance, double StepSize) const;
		void OnNumberOfStepsChanged(DynExp::ModuleInstance* Instance, int NumberOfSteps) const;
		void OnNumberOfRepetitionsChanged(DynExp::ModuleInstance* Instance, int NumberOfRepetitions) const;
		void OnScanBackAndForthToggled(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnStartAtToggled(DynExp::ModuleInstance* Instance, bool) const;
		void OnPathChanged(DynExp::ModuleInstance* Instance, const std::string& SaveFilename) const;
		void OnPathChanged(DynExp::ModuleInstance* Instance, const QString SaveFilename) const;
		void OnFinishedCapturing(DynExp::ModuleInstance* Instance) const;

		// State functions for state machine
		StateType ReadyStateFunc(DynExp::ModuleInstance& Instance);
		StateType WaitForSettingFrequencyStateFunc(DynExp::ModuleInstance& Instance);
		StateType WaitForCapturingStateFunc(DynExp::ModuleInstance& Instance);

		// States for state machine
		static constexpr auto ReadyState = Util::StateMachineState(StateType::Ready,
			&LaserScanningSpectroscopy::ReadyStateFunc, "Ready");
		static constexpr auto WaitForSettingFrequencyState = Util::StateMachineState(StateType::WaitForSettingFrequency,
			&LaserScanningSpectroscopy::WaitForSettingFrequencyStateFunc, "Laser stabilizes at target Frequency...");
		static constexpr auto WaitForCapturingState = Util::StateMachineState(StateType::WaitForCapturing,
			&LaserScanningSpectroscopy::WaitForCapturingStateFunc, "Capturing...");
		
		// Logical const-ness: allow events to set the state machine's current state.
		mutable Util::StateMachine<StateMachineStateType> StateMachine;

		// Indicates waiting time within measurements.
		mutable std::chrono::system_clock::time_point WaitingEndTimePoint;

		size_t NumFailedUpdateAttempts = 0;
	};
}