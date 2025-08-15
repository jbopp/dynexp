// This file is part of DynExp.

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../../MetaInstruments/Laser.h"
#include "../../Instruments/InterModuleCommunicator.h"

#include "CommonModuleEvents.h"
//#include "LaserScanningSpectroscopyEvents.h"

#include <QWidget>
#include "ui_LaserScanningSpectroscopy.h"

namespace DynExpModule::LaserScanningSpectroscopy
{
	class LaserScanningSpectroscopy;
	class LaserScanningSpectroscopyData;

	enum class StateType {
		Ready,
		WaitForSettingFrequency,
		WaitForCapturing,
	};

	using StateMachineStateType = Util::StateMachineState<StateType(LaserScanningSpectroscopy::*)(DynExp::ModuleInstance&)>;

	class LaserScanningSpectroscopyWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		LaserScanningSpectroscopyWidget(LaserScanningSpectroscopy& Owner, QModuleWidget* parent = nullptr);
		~LaserScanningSpectroscopyWidget() = default;

		bool AllowResize() const noexcept override final { return true; }
		const auto& GetUI() const noexcept { return ui; }

		void InitializeUI(Util::SynchronizedPointer<LaserScanningSpectroscopyData>& ModuleData);
		void UpdateUI(Util::SynchronizedPointer<LaserScanningSpectroscopyData>& ModuleData);
	
		Ui::LaserScanningSpectroscopy ui;
	};

	class LaserScanningSpectroscopyData : public DynExp::QModuleDataBase
	{
	public:
		LaserScanningSpectroscopyData() { Init(); }
		virtual ~LaserScanningSpectroscopyData() = default;

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }
		auto& GetPLECommunicator() { return PLECommunicator; }
		auto& GetWFCommunicator() { return WFCommunicator; }
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
		std::filesystem::path Filepath;
		
		StateType LaserScanningSpectroscopyState = StateType::Ready;
		double LaserScanningSpectroscopyProgress;
		DynExpInstr::LaserData::LaserStateType LaserState = DynExpInstr::LaserData::LaserStateType::Ready;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::Laser> Laser;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> PLECommunicator;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> WFCommunicator;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<LaserScanningSpectroscopyData>) {};

		void Init();
		bool UIInitialized;
	};

	class LaserScanningSpectroscopyParams : public DynExp::QModuleParamsBase
	{
	public:
		LaserScanningSpectroscopyParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~LaserScanningSpectroscopyParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "LaserScanningSpectroscopyParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::Laser>> Laser = { *this, GetCore().GetInstrumentManager(),
			"Laser", "Laser", "Underlying Laser instrument to be used as a data source", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> PLECommunicator = { *this, GetCore().GetInstrumentManager(),
			"PLEInterModuleCommunicator", "PLE inter-module communicator", "Inter-module communicator to control data aquisition", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> WFCommunicator = { *this, GetCore().GetInstrumentManager(),
			"WFInterModuleCommunicator", "WF inter-module communicator", "Inter-module communicator to communicate with WF module", DynExpUI::Icons::Instrument, true };
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

		constexpr static auto Name() noexcept { return "LaserScanningSpectroscopy"; }
		constexpr static auto Category() noexcept { return "Experiments"; }

		LaserScanningSpectroscopy(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		//	: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~LaserScanningSpectroscopy();// = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(10); }

	private:
		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;

		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		void FrequencyStep(DynExp::ModuleInstance* Instance) const;

		// Helper functions
		bool IsReadyState() const;
		bool IsSettingFrequencyState() const;
		bool IsCapturingState() const;
		std::filesystem::path BuildFilename(Util::SynchronizedPointer<ModuleDataType>& ModuleData, std::string_view FilenameSuffix) const;

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
		void OnStartAtMinimumToggled(DynExp::ModuleInstance* Instance, bool) const;
		void OnStartAtMaximumToggled(DynExp::ModuleInstance* Instance, bool) const;
		void OnScanBackAndForthToggled(DynExp::ModuleInstance* Instance, bool) const;
		void OnFinishedCapturing(DynExp::ModuleInstance* Instance) const;

		void OnStartClicked(DynExp::ModuleInstance* Instance, bool) const;								// Those function exist "twice" because Qt expects an additional parameter bool, the event does not
		void OnStopClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnStart(DynExp::ModuleInstance* Instance) const;
		void OnStop(DynExp::ModuleInstance* Instance) const;
		void OnPathChanged(DynExp::ModuleInstance* Instance, const std::string& SaveFilename) const;	// This function exists "twice" because Qt expects a QString while the SetFilenameEvent expects a std::string&
		void OnPath(DynExp::ModuleInstance* Instance, const QString SaveFilename) const;
		void OnPathBrowseClicked(DynExp::ModuleInstance* Instance, bool) const;

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

		size_t NumFailedUpdateAttempts = 0;
	};
}
