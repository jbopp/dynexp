// This file is part of DynExp.

/**
 * @file SmarAct.h
 * @brief Implementation of an instrument to control a single positioner stage connected to the
 * SmarAct MCS2.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterSmarAct.h"
#include "MetaInstruments/Stage.h"

namespace DynExpInstr
{
	class SmarAct;

	namespace SmarActTasks
	{
		class InitTask : public PositionerStageTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<PositionerStageTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;

			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public PositionerStageTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<PositionerStageTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;

			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public PositionerStageTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<PositionerStageTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;

			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class SetHomeTask final : public DynExp::TaskBase
		{
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class ReferenceTask final : public DynExp::TaskBase
		{
		public:
			ReferenceTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class CalibrateTask final : public DynExp::TaskBase
		{
		public:
			CalibrateTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetVelocityTask final : public DynExp::TaskBase
		{
		public:
			SetVelocityTask(PositionerStageData::PositionType Velocity) noexcept : Velocity(Velocity) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStageData::PositionType Velocity;
		};

		class MoveToHomeTask final : public DynExp::TaskBase
		{
		public:
			MoveToHomeTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class MoveAbsoluteTask final : public DynExp::TaskBase
		{
		public:
			MoveAbsoluteTask(PositionerStageData::PositionType Position, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Position(Position) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStageData::PositionType Position;
		};

		class MoveRelativeTask final : public DynExp::TaskBase
		{
		public:
			MoveRelativeTask(PositionerStageData::PositionType Position, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Position(Position) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStageData::PositionType Position;
		};

		class StopMotionTask final : public DynExp::TaskBase
		{
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class SmarActData : public PositionerStageData
	{
		friend class SmarActTasks::InitTask;
		friend class SmarActTasks::UpdateTask;

	public:
		struct SmarActChannelStatusType
		{
			constexpr void Set(int32_t ByteCode) noexcept { this->ByteCode = ByteCode; }

			constexpr bool IsSensorPresent() const noexcept { return ByteCode & SA_CTL_CH_STATE_BIT_SENSOR_PRESENT; }
			constexpr bool IsMoving() const noexcept { return ByteCode & (SA_CTL_CH_STATE_BIT_ACTIVELY_MOVING); }
			constexpr bool IsClosedLoopActive() const noexcept { return ByteCode & (SA_CTL_CH_STATE_BIT_CLOSED_LOOP_ACTIVE); }
			constexpr bool IsEndStopReached() const noexcept { return ByteCode & SA_CTL_CH_STATE_BIT_END_STOP_REACHED; }
			constexpr bool IsRangeLimitReached() const noexcept { return ByteCode & SA_CTL_CH_STATE_BIT_RANGE_LIMIT_REACHED; }
			constexpr bool IsReferencing() const noexcept { return ByteCode & SA_CTL_CH_STATE_BIT_REFERENCING; }
			constexpr bool IsCalibrating() const noexcept { return ByteCode & SA_CTL_CH_STATE_BIT_CALIBRATING; }
			constexpr bool IsReferenced() const noexcept { return ByteCode & SA_CTL_CH_STATE_BIT_IS_REFERENCED; }
			constexpr bool IsCalibrated() const noexcept { return ByteCode & SA_CTL_CH_STATE_BIT_IS_CALIBRATED; }

			constexpr bool IsErrorState() const noexcept
			{
				return ByteCode & (SA_CTL_CH_STATE_BIT_FOLLOWING_LIMIT_REACHED | SA_CTL_CH_STATE_BIT_MOVEMENT_FAILED
					| SA_CTL_CH_STATE_BIT_POSITIONER_OVERLOAD | SA_CTL_CH_STATE_BIT_OVER_TEMPERATURE
					| SA_CTL_CH_STATE_BIT_POSITIONER_FAULT);
			}

		private:
			int32_t ByteCode = 0;
		};

		SmarActData() = default;
		virtual ~SmarActData() = default;

		auto GetChannel() const noexcept { return Channel; }

		// Reference getter overload to allow other tasks than update task to update the channel state (especially if they started the stage to move).
		auto& GetSmarActChannelStatus() noexcept { return SmarActChannelStatus; }
		auto GetSmarActChannelStatus() const noexcept { return SmarActChannelStatus; }

		auto GetHomePosition() const noexcept { return HomePosition; }
		void SetHomePosition(PositionType HomePosition) noexcept { this->HomePosition = HomePosition; }

		DynExp::LinkedObjectWrapperContainer<DynExpHardware::SmarActHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<PositionerStageData>) override final;
		virtual void ResetImpl(dispatch_tag<SmarActData>) {};

		virtual bool IsMovingChild() const noexcept override { return SmarActChannelStatus.IsMoving(); }
		virtual bool HasArrivedChild() const noexcept override { return SmarActChannelStatus.IsClosedLoopActive() && !SmarActChannelStatus.IsMoving(); }	// SmarAct is holding target position.
		virtual bool HasFailedChild() const noexcept override { return SmarActChannelStatus.IsErrorState(); }
		virtual bool IsReferencedChild() const noexcept override { return SmarActChannelStatus.IsReferenced(); }

		DynExpHardware::SmarActHardwareAdapter::ChannelType Channel = 0;

		SmarActChannelStatusType SmarActChannelStatus;
		size_t NumFailedStatusUpdateAttempts = 0;
		PositionType HomePosition = 0;
	};

	class SmarActParams : public PositionerStageParams
	{
	public:
		SmarActParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : PositionerStageParams(ID, Core) {}
		virtual ~SmarActParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SmarActParams"; }

		Param<DynExp::ObjectLink<DynExpHardware::SmarActHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "SmarAct controller", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<ParamsConfigDialog::NumberType> Channel = { *this, "Channel", "Channel",
			"Channel of the SmarAct controller this instrument refers to", true, 0, 0, std::numeric_limits<uint8_t>::max(), 1, 0 };

	private:
		void ConfigureParamsImpl(dispatch_tag<PositionerStageParams>) override final { ConfigureParamsImpl(dispatch_tag<SmarActParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<SmarActParams>) {}
	};

	class SmarActConfigurator : public PositionerStageConfigurator
	{
	public:
		using ObjectType = SmarAct;
		using ParamsType = SmarActParams;

		SmarActConfigurator() = default;
		virtual ~SmarActConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<SmarActConfigurator>(ID, Core); }
	};

	class SmarAct : public PositionerStage
	{
	public:
		using ParamsType = SmarActParams;
		using ConfigType = SmarActConfigurator;
		using InstrumentDataType = SmarActData;

		constexpr static auto Name() noexcept { return "SmarAct"; }

		SmarAct(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~SmarAct() {}

		virtual std::string GetName() const override { return Name(); }

		virtual PositionerStageData::PositionType GetMinPosition() const noexcept override { return -100e9; }
		virtual PositionerStageData::PositionType GetMaxPosition() const noexcept override { return 100e9; }
		virtual PositionerStageData::PositionType GetResolution() const noexcept override { return 10; }
		virtual PositionerStageData::PositionType GetMinVelocity() const noexcept override { return 1; }
		virtual PositionerStageData::PositionType GetMaxVelocity() const noexcept override { return 100e6; }
		virtual PositionerStageData::PositionType GetDefaultVelocity() const noexcept override { return 1e6; }
		virtual double GetStepNanoMeterRatio() const noexcept override { return 1e-3; }		// pm to nm
		virtual bool IsUsingSIUnits() const noexcept override { return true; }				// Assuming SmarAct positioners with sensor

		virtual void SetHome() const override { MakeAndEnqueueTask<SmarActTasks::SetHomeTask>(); }
		virtual void Reference([[maybe_unused]] DirectionType Direction = DirectionType::Forward, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SmarActTasks::ReferenceTask>(CallbackFunc); }
		virtual void Calibrate(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SmarActTasks::CalibrateTask>(CallbackFunc); }
		virtual void SetVelocity(PositionerStageData::PositionType Velocity) const override { MakeAndEnqueueTask<SmarActTasks::SetVelocityTask>(Velocity); }

		virtual void MoveToHome(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SmarActTasks::MoveToHomeTask>(CallbackFunc); }
		virtual void MoveAbsolute(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SmarActTasks::MoveAbsoluteTask>(Position, CallbackFunc); }
		virtual void MoveRelative(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SmarActTasks::MoveRelativeTask>(Position, CallbackFunc); }
		virtual void StopMotion() const override { MakeAndEnqueueTask<SmarActTasks::StopMotionTask>(); }

	private:
		virtual void OnErrorChild() const override;

		void ResetImpl(dispatch_tag<PositionerStage>) override final;
		virtual void ResetImpl(dispatch_tag<SmarAct>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<SmarActTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<SmarActTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<SmarActTasks::UpdateTask>(); }
	};
}