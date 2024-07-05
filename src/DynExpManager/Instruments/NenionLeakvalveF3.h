// This file is part of DynExp.

/**
 * @file NenionLeakvalveF3.h
 * @brief Implementation of an instrument to control the positioner stage of the
 * Nenion Leakvalve F3 valve.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/Stage.h"

namespace DynExpInstr
{
	class NenionLeakvalveF3;

	namespace NenionLeakvalveF3Tasks
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

	class NenionLeakvalveF3Data : public PositionerStageData
	{
		friend class NenionLeakvalveF3Tasks::UpdateTask;

	public:
		enum class NenionLeakvalveF3StatusType { Ready, Moving, Arrived, LocalMode, AnalogMode, Disabled };

		NenionLeakvalveF3Data() = default;
		virtual ~NenionLeakvalveF3Data() = default;

		auto GetNenionLeakvalveF3Status() const noexcept { return NenionLeakvalveF3Status; }

		auto GetHomePosition() const noexcept { return HomePosition; }
		void SetHomePosition(PositionType HomePosition) noexcept { this->HomePosition = HomePosition; }

		DynExp::LinkedObjectWrapperContainer<DynExp::SerialCommunicationHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<PositionerStageData>) override final;
		virtual void ResetImpl(dispatch_tag<NenionLeakvalveF3Data>) {};

		virtual bool IsMovingChild() const noexcept override { return NenionLeakvalveF3Status == NenionLeakvalveF3StatusType::Moving; }
		virtual bool HasArrivedChild() const noexcept override { return NenionLeakvalveF3Status == NenionLeakvalveF3StatusType::Arrived; }
		virtual bool HasFailedChild() const noexcept override;
		virtual bool IsReferencedChild() const noexcept override { return true; }

		NenionLeakvalveF3StatusType NenionLeakvalveF3Status;
		size_t NumFailedStatusUpdateAttempts = 0;
		PositionType HomePosition = 0;
	};

	class NenionLeakvalveF3Params : public PositionerStageParams
	{
	public:
		NenionLeakvalveF3Params(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : PositionerStageParams(ID, Core) {}
		virtual ~NenionLeakvalveF3Params() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NenionLeakvalveF3Params"; }

		Param<DynExp::ObjectLink<DynExp::SerialCommunicationHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "Serial port", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };

	private:
		void ConfigureParamsImpl(dispatch_tag<PositionerStageParams>) override final { ConfigureParamsImpl(dispatch_tag<NenionLeakvalveF3Params>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<NenionLeakvalveF3Params>) {}
	};

	class NenionLeakvalveF3Configurator : public PositionerStageConfigurator
	{
	public:
		using ObjectType = NenionLeakvalveF3;
		using ParamsType = NenionLeakvalveF3Params;

		NenionLeakvalveF3Configurator() = default;
		virtual ~NenionLeakvalveF3Configurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NenionLeakvalveF3Configurator>(ID, Core); }
	};

	class NenionLeakvalveF3 : public PositionerStage
	{
	public:
		using ParamsType = NenionLeakvalveF3Params;
		using ConfigType = NenionLeakvalveF3Configurator;
		using InstrumentDataType = NenionLeakvalveF3Data;

		constexpr static auto Name() noexcept { return "Nenion Leakvalve F3"; }

		NenionLeakvalveF3(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NenionLeakvalveF3() {}

		virtual std::string GetName() const override { return Name(); }

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(200); }

		virtual PositionerStageData::PositionType GetMinPosition() const noexcept override { return 1; }
		virtual PositionerStageData::PositionType GetMaxPosition() const noexcept override { return 40000; }
		virtual PositionerStageData::PositionType GetResolution() const noexcept override { return 1; }
		virtual PositionerStageData::PositionType GetMinVelocity() const noexcept override { return 10; }
		virtual PositionerStageData::PositionType GetMaxVelocity() const noexcept override { return 40000; }
		virtual PositionerStageData::PositionType GetDefaultVelocity() const noexcept override { return 4000; }

		PositionerStageData::PositionType EnforcePositionLimits(PositionerStageData::PositionType Position) const;

		virtual void SetHome() const override { MakeAndEnqueueTask<NenionLeakvalveF3Tasks::SetHomeTask>(); }
		virtual void Reference([[maybe_unused]] DirectionType Direction = DirectionType::Forward, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NenionLeakvalveF3Tasks::ReferenceTask>(CallbackFunc); }
		virtual void SetVelocity(PositionerStageData::PositionType Velocity) const override { MakeAndEnqueueTask<NenionLeakvalveF3Tasks::SetVelocityTask>(Velocity); }

		virtual void MoveToHome(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NenionLeakvalveF3Tasks::MoveToHomeTask>(CallbackFunc); }
		virtual void MoveAbsolute(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NenionLeakvalveF3Tasks::MoveAbsoluteTask>(Position, CallbackFunc); }
		virtual void MoveRelative(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NenionLeakvalveF3Tasks::MoveRelativeTask>(Position, CallbackFunc); }
		virtual void StopMotion() const override { MakeAndEnqueueTask<NenionLeakvalveF3Tasks::StopMotionTask>(); }

	private:
		virtual void OnErrorChild() const override;

		void ResetImpl(dispatch_tag<PositionerStage>) override final;
		virtual void ResetImpl(dispatch_tag<NenionLeakvalveF3>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NenionLeakvalveF3Tasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NenionLeakvalveF3Tasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NenionLeakvalveF3Tasks::UpdateTask>(); }
	};
}