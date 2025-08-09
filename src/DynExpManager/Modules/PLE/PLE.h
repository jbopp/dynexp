// This file is part of DynExp.

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../../MetaInstruments/Laser.h"
#include "../../Instruments/InterModuleCommunicator.h"

//#include "PLEEvents.h"

#include <QWidget>
#include "ui_PLE.h"

namespace DynExpModule::PLE
{
	class PLE;
	class PLEData;

	enum class StateType {
		Ready,
		LaserInit,
		WaitForSettingFrequency,
		PLEStep,
		WaitForCapturing,
	};

	using StateMachineStateType = Util::StateMachineState<StateType(PLE::*)(DynExp::ModuleInstance&)>;

	class PLEWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		PLEWidget(PLE& Owner, QModuleWidget* parent = nullptr);
		~PLEWidget() = default;

		bool AllowResize() const noexcept override final { return true; }
		const auto& GetUI() const noexcept { return ui; }

		void InitializeUI(Util::SynchronizedPointer<PLEData>& ModuleData);
		void UpdateUI(Util::SynchronizedPointer<PLEData>& ModuleData);
	
		Ui::PLE ui;
	};

	class PLEData : public DynExp::QModuleDataBase
	{
	public:
		PLEData() { Init(); }
		virtual ~PLEData() = default;

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }
		//auto& GetPLE() { return PLE; }
		auto& GetCommunicator() { return Communicator; }
		auto& GetLaser() { return Laser; }

		double LowerFrequencyLimit;
		double UpperFrequencyLimit;
		double FrequencyRange;
		double ModeHopFreeTuningRange;
		double CenterFrequency;
		double Stepsize;
		double NumberOfSteps;
		double Repetitions;
		double StartingPoint;
		double EndingPoint;
		bool ScanBackAndForth = false;
		int StepCount = 0;
		int RepCount = 0;
		
		StateType PLEState = StateType::Ready;
		double PLEProgress;
		DynExpInstr::LaserData::LaserStateType LaserState = DynExpInstr::LaserData::LaserStateType::Ready;

		//std::filesystem::path GetAutoMeasureSavePath() const;
		//void SetAutoMeasureSavePath(std::filesystem::path SavePath) noexcept { AutoMeasureSavePath = SavePath; }

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::Laser> Laser;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> Communicator;
		
	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<PLEData>) {};

		void Init();
		bool UIInitialized;
	};

	class PLEParams : public DynExp::QModuleParamsBase
	{
	public:
		PLEParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~PLEParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "PLEParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::Laser>> Laser = { *this, GetCore().GetInstrumentManager(),
			"Laser", "Laser", "Underlying Laser instrument to be used as a data source", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> Communicator = { *this, GetCore().GetInstrumentManager(),
			"InterModuleCommunicator", "Inter-module communicator", "Inter-module communicator to control this module with", DynExpUI::Icons::Instrument, true };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class PLEConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = PLE;
		using ParamsType = PLEParams;

		PLEConfigurator() = default;
		virtual ~PLEConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<PLEConfigurator>(ID, Core); }
	};

	class PLE : public DynExp::QModuleBase
	{
	public:
		using ParamsType = PLEParams;
		using ConfigType = PLEConfigurator;
		using ModuleDataType = PLEData;

		constexpr static auto Name() noexcept { return "PLE"; }
		constexpr static auto Category() noexcept { return "Experiments"; }

		PLE(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		//	: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~PLE();// = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(10); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Helper functions
		bool IsReadyState() const;
		bool IsSettingFrequencyState() const;
		//void StartCapturing(Util::SynchronizedPointer<ModuleDataType>& ModuleData, const FinishedSettingFrequencyEvent& Event) const;
		bool IsCapturingState() const;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnLowerFrequencyLimitChanged(DynExp::ModuleInstance* Instance, const double LowerFrequencyLimit) const;
		void OnUpperFrequencyLimitChanged(DynExp::ModuleInstance* Instance, const double UpperFrequencyLimit) const;
		void OnFrequencyRangeChanged(DynExp::ModuleInstance* Instance, const double FrequencyRange) const;
		void OnFrequencyCenterChanged(DynExp::ModuleInstance* Instance, const double FrequencyCenter) const;
		void OnStepsizeChanged(DynExp::ModuleInstance* Instance, const double Stepsize) const;
		void OnRepetitionsChanged(DynExp::ModuleInstance* Instance, const int Repititions) const;
		void OnNumberOfStepsChanged(DynExp::ModuleInstance* Instance, const int NumberOfSteps) const;
		void OnStartAtMinimumToggled(DynExp::ModuleInstance* Instance) const;
		void OnStartAtMaximumToggled(DynExp::ModuleInstance* Instance) const;
		void OnScanBackAndForthToggled(DynExp::ModuleInstance* Instance) const;

		void OnStartClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnStopClicked(DynExp::ModuleInstance* Instance, bool) const;

		// State functions for state machine
		StateType ReadyStateFunc(DynExp::ModuleInstance& Instance);
		StateType LaserInitStateFunc(DynExp::ModuleInstance& Instance);
		StateType WaitForSettingFrequencyStateFunc(DynExp::ModuleInstance& Instance);
		StateType PLEStepStateFunc(DynExp::ModuleInstance& Instance);
		StateType WaitForCapturingStateFunc(DynExp::ModuleInstance& Instance);

		// States for state machine
		static constexpr auto ReadyState = Util::StateMachineState(StateType::Ready,
			&PLE::ReadyStateFunc, "Ready");
		static constexpr auto LaserInitState = Util::StateMachineState(StateType::LaserInit,
			&PLE::LaserInitStateFunc, "Laser parameters initialized.");
		static constexpr auto LaserSettingFrequencyState = Util::StateMachineState(StateType::WaitForSettingFrequency,
			&PLE::WaitForSettingFrequencyStateFunc, "Laser stabilizes at target Frequency...");
		static constexpr auto FinishedCapturingState = Util::StateMachineState(StateType::WaitForCapturing,
			&PLE::WaitForCapturingStateFunc, "Capturing...");
		static constexpr auto PLEStepState = Util::StateMachineState(StateType::PLEStep,
			&PLE::PLEStepStateFunc, "Moving to next Frequency...");

		// Logical const-ness: allow events to set the state machine's current state.
		mutable Util::StateMachine<StateMachineStateType> StateMachine;

		size_t NumFailedUpdateAttempts = 0;
	};
}
