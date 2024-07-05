// This file is part of DynExp.

/**
 * @file Trajectory1D.h
 * @brief Implementation of a module to interpret samples stored in a data stream instrument as
 * trajectories of 1D positioners and to move the positioners along those trajectories.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/Stage.h"
#include "../MetaInstruments/DataStreamInstrument.h"
#include "../Instruments/InterModuleCommunicator.h"

#include "CommonModuleEvents.h"

#include <QWidget>
#include "ui_Trajectory1D.h"

namespace DynExpModule
{
	class Trajectory1D;

	class Trajectory1DWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		Trajectory1DWidget(Trajectory1D& Owner, QModuleWidget* parent = nullptr);
		~Trajectory1DWidget() = default;

		bool AllowResize() const noexcept override final { return false; }

		const auto& GetUI() const noexcept { return ui; }

	private:
		Ui::Trajectory1D ui;
	};

	class Trajectory1DData : public DynExp::QModuleDataBase
	{
	public:
		enum TriggerModeType { Continuous, ManualOnce, Manual, OnStreamChanged };

		Trajectory1DData() { Init(); }
		virtual ~Trajectory1DData() = default;

		auto& GetTrajectoryDataInstr() { return TrajectoryDataInstr; }
		auto& GetPositionerStage() { return PositionerStage; }
		auto& GetCommunicator() { return Communicator; }

		auto GetTriggerMode() const noexcept { return TriggerMode; }
		void SetTriggerMode(TriggerModeType TriggerMode) noexcept { this->TriggerMode = TriggerMode; }
		auto GetRepeatCount() const noexcept { return RepeatCount; }
		void SetRepeatCount(size_t RepeatCount) noexcept { this->RepeatCount = RepeatCount; }
		auto GetDwellTime() const noexcept { return DwellTime; }
		void SetDwellTime(std::chrono::milliseconds DwellTime) noexcept { this->DwellTime = DwellTime; }

		auto GetLastWrittenSampleID() const noexcept { return LastWrittenSampleID; }
		void SetLastWrittenSampleID(size_t LastWrittenSampleID) noexcept { this->LastWrittenSampleID = LastWrittenSampleID; }
		const auto& GetSamples() const noexcept { return Samples; }
		auto& GetSamples() noexcept { return Samples; }
		void SetSamples(DynExpInstr::DataStreamBase::BasicSampleListType&& Samples) noexcept { this->Samples = std::move(Samples); }

		auto GetCurrentPlaybackPos() const noexcept { return CurrentPlaybackPos; }
		bool IsTriggered() const noexcept { return CurrentPlaybackPos > 0; }
		void SetCurrentPlaybackPos(size_t CurrentPlaybackPos) noexcept { this->CurrentPlaybackPos = CurrentPlaybackPos; }
		auto GetCurrentRepeatCount() const noexcept { return CurrentRepeatCount; }
		void SetCurrentRepeatCount(size_t CurrentRepeatCount) noexcept { this->CurrentRepeatCount = CurrentRepeatCount; }
		auto IsReady() const noexcept { return Ready; }
		void SetReady(bool Ready) noexcept { this->Ready = Ready; }
		auto GetTrajectoryStartedTime() const noexcept { return TrajectoryStartedTime; }
		void SetTrajectoryStartedTime(std::chrono::time_point<std::chrono::system_clock> TrajectoryStartedTime) noexcept { this->TrajectoryStartedTime = TrajectoryStartedTime; }

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<Trajectory1DData>) {};

		void Init();

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::DataStreamInstrument> TrajectoryDataInstr;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::PositionerStage> PositionerStage;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> Communicator;

		TriggerModeType TriggerMode = TriggerModeType::Manual;
		size_t RepeatCount = 1;
		std::chrono::milliseconds DwellTime = std::chrono::milliseconds(100);

		size_t LastWrittenSampleID = 0;
		DynExpInstr::DataStreamBase::BasicSampleListType Samples;

		size_t CurrentPlaybackPos = 0;	//!< 0 means waiting for trigger. Values > 0 mean running. They indicate the sample to be written next.
		size_t CurrentRepeatCount = 0;
		bool Ready = false;				//!< Running (not necessarily triggered yet) if true.
		std::chrono::time_point<std::chrono::system_clock> TrajectoryStartedTime;
	};

	class Trajectory1DParams : public DynExp::QModuleParamsBase
	{
	public:
		static Util::TextValueListType<Trajectory1DData::TriggerModeType> TriggerModeTypeStrList();

		Trajectory1DParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~Trajectory1DParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "Trajectory1DParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>> TrajectoryDataInstr = { *this, GetCore().GetInstrumentManager(),
			"TrajectoryDataInstr", "Trajectory data stream", "Data stream instrument to stream position data from", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::PositionerStage>> PositionerStage = { *this, GetCore().GetInstrumentManager(),
			"PositionerStage", "Positioner", "Underlying positioner to be controlled by this module", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> Communicator = { *this, GetCore().GetInstrumentManager(),
			"InterModuleCommunicator", "Inter-module communicator", "Inter-module communicator to control this module with", DynExpUI::Icons::Instrument, true };

		Param<Trajectory1DData::TriggerModeType> TriggerMode = { *this, TriggerModeTypeStrList(), "TriggerMode", "Trigger mode",
			"Trigger action which starts streaming the position data", true, Trajectory1DData::TriggerModeType::Manual };
		Param<ParamsConfigDialog::NumberType> RepeatCount = { *this, "RepeatCount", "Number of repetitions",
			"Determines how many times the trajectory data stream should be played back after a trigger event has occurred", true, 1, 1 };
		Param<ParamsConfigDialog::NumberType> DwellTime = { *this, "DwellTime", "Dwell time in ms",
			"Dwell time used for data streams containing samples without time data", true, 100, 1 };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class Trajectory1DConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = Trajectory1D;
		using ParamsType = Trajectory1DParams;

		Trajectory1DConfigurator() = default;
		virtual ~Trajectory1DConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<Trajectory1DConfigurator>(ID, Core); }
	};

	class Trajectory1D : public DynExp::QModuleBase
	{
	public:
		using ParamsType = Trajectory1DParams;
		using ConfigType = Trajectory1DConfigurator;
		using ModuleDataType = Trajectory1DData;

		constexpr static auto Name() noexcept { return "1D Trajectory Positioning"; }
		constexpr static auto Category() noexcept { return "Motion Control"; }

		Trajectory1D(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~Trajectory1D() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(1); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Helper functions
		void UpdateStream(Util::SynchronizedPointer<ModuleDataType>& ModuleData);
		void Move(Util::SynchronizedPointer<ModuleDataType>& ModuleData);
		void Start(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void Stop(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void Trigger(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;

		void OnStartClicked(DynExp::ModuleInstance* Instance, bool) const { OnStart(Instance); }
		void OnStopClicked(DynExp::ModuleInstance* Instance, bool) const { OnStop(Instance); }
		void OnTriggerClicked(DynExp::ModuleInstance* Instance, bool) const { OnTrigger(Instance); }
		void OnStart(DynExp::ModuleInstance* Instance) const;
		void OnStop(DynExp::ModuleInstance* Instance) const;
		void OnTrigger(DynExp::ModuleInstance* Instance) const;

		size_t NumFailedUpdateAttempts = 0;
	};
}