// This file is part of DynExp.

/**
 * @file PI-C-862.h
 * @brief Implementation of an instrument to control the Physik Instrumente (PI) C-862
 * positioner stage.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/Stage.h"

namespace DynExpInstr
{
	class PI_C_862;

	namespace PI_C_862_Tasks
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
			ReferenceTask(PositionerStage::DirectionType Direction, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Direction(Direction) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStage::DirectionType Direction;
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

	class PI_C_862StageData : public PositionerStageData
	{
		friend class PI_C_862_Tasks::UpdateTask;

	public:
		struct LM629StatusType
		{
			constexpr void Set(uint8_t ByteCode) noexcept { this->ByteCode = ByteCode; }

			constexpr bool Busy() const noexcept { return ByteCode & (1 << 0); }
			constexpr bool CommandError() const noexcept { return ByteCode & (1 << 1); }
			constexpr bool TrajectoryComplete() const noexcept { return ByteCode & (1 << 2); }
			constexpr bool IndexPulseReceived() const noexcept { return ByteCode & (1 << 3); }
			constexpr bool PositionLimitExceeded() const noexcept { return ByteCode & (1 << 4); }
			constexpr bool ExcessivePositionError() const noexcept { return ByteCode & (1 << 5); }
			constexpr bool BreakpointReached() const noexcept { return ByteCode & (1 << 6); }
			constexpr bool MotorLoopOff() const noexcept { return ByteCode & (1 << 7); }

		private:
			uint8_t ByteCode = 0;
		};

		enum ErrorCodeType : uint8_t {
			NoError,
			CommandNotFound,
			FirstCommandCharacterWasNotALetter,
			CharacterFollowingCommandWasNotADigit = 5,
			ValueTooLarge,
			ValueTooSmall,
			ContinuationCharacterWasNotAComma,
			CommandBufferOverflow,
			MacroStorageOverflow
		};

		PI_C_862StageData() = default;
		virtual ~PI_C_862StageData() = default;

		auto GetLM629Status() const noexcept { return LM629Status; }
		auto GetErrorCode() const noexcept { return ErrorCode; }

		DynExp::LinkedObjectWrapperContainer<DynExp::SerialCommunicationHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<PositionerStageData>) override final;
		virtual void ResetImpl(dispatch_tag<PI_C_862StageData>) {};

		virtual bool IsMovingChild() const noexcept override;
		virtual bool HasArrivedChild() const noexcept override;
		virtual bool HasFailedChild() const noexcept override;

		LM629StatusType LM629Status;
		ErrorCodeType ErrorCode = NoError;
		size_t NumFailedStatusUpdateAttempts = 0;
	};

	class PI_C_862_Params : public PositionerStageParams
	{
	public:
		PI_C_862_Params(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : PositionerStageParams(ID, Core) {}
		virtual ~PI_C_862_Params() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "PI_C_862_Params"; }

		Param<DynExp::ObjectLink<DynExp::SerialCommunicationHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "Serial port", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<ParamsConfigDialog::TextType> MercuryAddress = { *this, "MercuryAddress", "Mercury address",
			"Address (0-F) of the Mercury controller to be used", true, "0" };

	private:
		void ConfigureParamsImpl(dispatch_tag<PositionerStageParams>) override final { ConfigureParamsImpl(dispatch_tag<PI_C_862_Params>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<PI_C_862_Params>) {}
	};

	class PI_C_862_Configurator : public PositionerStageConfigurator
	{
	public:
		using ObjectType = PI_C_862;
		using ParamsType = PI_C_862_Params;

		PI_C_862_Configurator() = default;
		virtual ~PI_C_862_Configurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<PI_C_862_Configurator>(ID, Core); }
	};

	class PI_C_862 : public PositionerStage
	{
	public:
		using ParamsType = PI_C_862_Params;
		using ConfigType = PI_C_862_Configurator;
		using InstrumentDataType = PI_C_862StageData;

		static std::string AnswerToNumberString(std::string&& Answer, const char* StartCode);

		constexpr static auto Name() noexcept { return "PI C-862"; }

		PI_C_862(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~PI_C_862() {}

		virtual std::string GetName() const override { return Name(); }

		virtual PositionerStageData::PositionType GetMinPosition() const noexcept override { return -12455000; }
		virtual PositionerStageData::PositionType GetMaxPosition() const noexcept override { return 12455000; }
		virtual PositionerStageData::PositionType GetResolution() const noexcept override { return 1000; }
		virtual PositionerStageData::PositionType GetMinVelocity() const noexcept override { return 0; }
		virtual PositionerStageData::PositionType GetMaxVelocity() const noexcept override { return 500000; }
		virtual PositionerStageData::PositionType GetDefaultVelocity() const noexcept override { return 200000; }

		virtual void SetHome() const override { MakeAndEnqueueTask<PI_C_862_Tasks::SetHomeTask>(); }
		virtual void Reference(DirectionType Direction = DirectionType::Forward, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<PI_C_862_Tasks::ReferenceTask>(Direction, CallbackFunc); }
		virtual void SetVelocity(PositionerStageData::PositionType Velocity) const override { MakeAndEnqueueTask<PI_C_862_Tasks::SetVelocityTask>(Velocity); }

		virtual void MoveToHome(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<PI_C_862_Tasks::MoveToHomeTask>(CallbackFunc); }
		virtual void MoveAbsolute(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<PI_C_862_Tasks::MoveAbsoluteTask>(Position, CallbackFunc); }
		virtual void MoveRelative(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<PI_C_862_Tasks::MoveRelativeTask>(Position, CallbackFunc); }
		virtual void StopMotion() const override { MakeAndEnqueueTask<PI_C_862_Tasks::StopMotionTask>(); }

	private:
		virtual void OnErrorChild() const override;

		void ResetImpl(dispatch_tag<PositionerStage>) override final;
		virtual void ResetImpl(dispatch_tag<PI_C_862>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<PI_C_862_Tasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<PI_C_862_Tasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<PI_C_862_Tasks::UpdateTask>(); }
	};
}