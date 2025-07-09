// This file is part of DynExp.

/**
 * @file NP_Conex_CC.h
 * @brief Implementation of an instrument to control the Newport rotation stage with the NP_Conex_CC controller.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/Stage.h"

namespace DynExpInstr
{
	class NP_Conex_CC;

	namespace NP_Conex_CC_Tasks
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

		class ResetTask final : public DynExp::TaskBase
		{
		public:
			ResetTask(CallbackType CallbackFunc = nullptr, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
				: TaskBase(CallbackFunc, DeferUntil) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetReadyTask final : public DynExp::TaskBase
		{
		public:
			SetReadyTask(CallbackType CallbackFunc = nullptr, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
				: TaskBase(CallbackFunc, DeferUntil) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetHomeTask final : public DynExp::TaskBase
		{
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetHomeExecutionTask final : public DynExp::TaskBase
		{
		public:
			SetHomeExecutionTask(CallbackType CallbackFunc, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
				: TaskBase(CallbackFunc, DeferUntil) {}

		private:
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

		class SetReferenceExecutionTask final : public DynExp::TaskBase
		{
		public:
			SetReferenceExecutionTask(CallbackType CallbackFunc, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
				: TaskBase(CallbackFunc, DeferUntil) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetVelocityTask final : public DynExp::TaskBase
		{
		public:
			SetVelocityTask(PositionerStageData::PositionType Velocity, CallbackType CallbackFunc = nullptr, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
				: TaskBase(CallbackFunc, DeferUntil), Velocity(Velocity) {}

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

		class MoveToHomeExecutionTask final : public DynExp::TaskBase
		{
		public:
			MoveToHomeExecutionTask(CallbackType CallbackFunc, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
				: TaskBase(CallbackFunc, DeferUntil) {}

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

		class MoveAbsoluteExecutionTask final : public DynExp::TaskBase
		{
		public:
			MoveAbsoluteExecutionTask(PositionerStageData::PositionType Position, CallbackType CallbackFunc, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
				: TaskBase(CallbackFunc, DeferUntil), Position(Position) {}

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

		class MoveRelativeExecutionTask final : public DynExp::TaskBase
		{
		public:
			MoveRelativeExecutionTask(PositionerStageData::PositionType Position, CallbackType CallbackFunc, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
				: TaskBase(CallbackFunc, DeferUntil), Position(Position) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStageData::PositionType Position;

		};

		class StopMotionTask final : public DynExp::TaskBase
		{
		public:
			StopMotionTask(CallbackType CallbackFunc = nullptr, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
				: TaskBase(CallbackFunc, DeferUntil) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class NP_Conex_CCStageData : public PositionerStageData
	{
		friend class NP_Conex_CC_Tasks::UpdateTask;

	public:
		struct Conex_CCStatusType
		{
			constexpr void Set(uint8_t byteCode) noexcept { ByteCode = byteCode; }

			// NOT REFERENCED STATES
			constexpr bool NotReferencedFromReset() const noexcept { return ByteCode == 0x0A; }
			constexpr bool NotReferencedFromHoming() const noexcept { return ByteCode == 0x0B; }
			constexpr bool NotReferencedFromConfiguration() const noexcept { return ByteCode == 0x0C; }
			constexpr bool NotReferencedFromDisable() const noexcept { return ByteCode == 0x0D; }
			constexpr bool NotReferencedFromReady() const noexcept { return ByteCode == 0x0E; }
			constexpr bool NotReferencedFromMoving() const noexcept { return ByteCode == 0x0F; }
			constexpr bool NotReferencedNoParams() const noexcept { return ByteCode == 0x10; }

			// CONFIGURATION
			constexpr bool Configuration() const noexcept { return ByteCode == 0x14; }

			// HOMING
			constexpr bool Homing() const noexcept { return ByteCode == 0x1E; }

			// MOVING
			constexpr bool Moving() const noexcept { return ByteCode == 0x28; }

			// READY STATES
			constexpr bool ReadyFromHoming() const noexcept { return ByteCode == 0x32; }
			constexpr bool ReadyFromMoving() const noexcept { return ByteCode == 0x33; }
			constexpr bool ReadyFromDisable() const noexcept { return ByteCode == 0x34; }

			// READY T STATES
			constexpr bool ReadyTFromReady() const noexcept { return ByteCode == 0x36; }
			constexpr bool ReadyTFromTracking() const noexcept { return ByteCode == 0x37; }
			constexpr bool ReadyTFromDisableT() const noexcept { return ByteCode == 0x38; }

			// DISABLE STATES
			constexpr bool DisableFromReady() const noexcept { return ByteCode == 0x3C; }
			constexpr bool DisableFromMoving() const noexcept { return ByteCode == 0x3D; }
			constexpr bool DisableFromTracking() const noexcept { return ByteCode == 0x3E; }
			constexpr bool DisableFromReadyT() const noexcept { return ByteCode == 0x3F; }

			// TRACKING STATES
			constexpr bool TrackingFromReadyT() const noexcept { return ByteCode == 0x46; }
			constexpr bool TrackingFromTracking() const noexcept { return ByteCode == 0x47; }

		private:
			uint8_t ByteCode = 0;
		};


		enum ErrorCodeType : uint8_t {
			NoError,
			Error // T: Not defined yet
		};

		NP_Conex_CCStageData() = default;
		virtual ~NP_Conex_CCStageData() = default;

		auto GetConex_CCStatus() const noexcept { return Conex_CCStatus; }
		auto GetErrorCode() const noexcept { return ErrorCode; }

		DynExp::LinkedObjectWrapperContainer<DynExp::SerialCommunicationHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<PositionerStageData>) override final;
		virtual void ResetImpl(dispatch_tag<NP_Conex_CCStageData>) {};

		virtual bool IsMovingChild() const noexcept override;
		virtual bool HasArrivedChild() const noexcept override;
		virtual bool HasFailedChild() const noexcept override;

		Conex_CCStatusType Conex_CCStatus;
		ErrorCodeType ErrorCode = NoError;
		size_t NumFailedStatusUpdateAttempts = 0;
	};

	class NP_Conex_CC_Params : public PositionerStageParams
	{
	public:
		NP_Conex_CC_Params(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : PositionerStageParams(ID, Core) {}
		virtual ~NP_Conex_CC_Params() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NP_Conex_CC_Params"; }

		Param<DynExp::ObjectLink<DynExp::SerialCommunicationHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "Serial port", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<ParamsConfigDialog::NumberType> ConexAddress = { *this, "ConexAddress", "Conex address",
			"Address (1-31) of the Conex controller to be used", true, 1, 31 };

	private:
		void ConfigureParamsImpl(dispatch_tag<PositionerStageParams>) override final { ConfigureParamsImpl(dispatch_tag<NP_Conex_CC_Params>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<NP_Conex_CC_Params>) {}
	};

	class NP_Conex_CC_Configurator : public PositionerStageConfigurator
	{
	public:
		using ObjectType = NP_Conex_CC;
		using ParamsType = NP_Conex_CC_Params;

		NP_Conex_CC_Configurator() = default;
		virtual ~NP_Conex_CC_Configurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NP_Conex_CC_Configurator>(ID, Core); }
	};

	class NP_Conex_CC : public PositionerStage
	{
	public:
		using ParamsType = NP_Conex_CC_Params;
		using ConfigType = NP_Conex_CC_Configurator;
		using InstrumentDataType = NP_Conex_CCStageData;

		static std::string AnswerToNumberString(std::string&& Answer, const char* StartCode);

		constexpr static auto Name() noexcept { return "NP Conex-CC"; }

		NP_Conex_CC(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NP_Conex_CC() {}

		virtual std::string GetName() const override { return Name(); }

		virtual PositionerStageData::PositionType GetMinPosition() const noexcept override { return -180e6; }
		virtual PositionerStageData::PositionType GetMaxPosition() const noexcept override { return 180e6; }
		virtual PositionerStageData::PositionType GetResolution() const noexcept override { return 1; }
		virtual PositionerStageData::PositionType GetMinVelocity() const noexcept override { return 0; }
		virtual PositionerStageData::PositionType GetMaxVelocity() const noexcept override { return 1e17; }
		virtual PositionerStageData::PositionType GetDefaultVelocity() const noexcept override { return 10e6; }
		virtual double GetFloatToIntConversion() const noexcept { return 1e6; } // the controller expects a float as position with 6 digits of precision

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(1000); }

		virtual void SetHome() const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::SetHomeTask>(); }
		virtual void Reference(DirectionType Direction = DirectionType::Forward, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::ReferenceTask>(Direction, CallbackFunc); }
		virtual void SetVelocity(PositionerStageData::PositionType Velocity) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::SetVelocityTask>(Velocity); }

		virtual void MoveToHome(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::MoveToHomeTask>(CallbackFunc); }
		virtual void MoveAbsolute(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::MoveAbsoluteTask>(Position, CallbackFunc); }
		virtual void MoveRelative(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::MoveRelativeTask>(Position, CallbackFunc); }
		virtual void StopMotion() const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::StopMotionTask>(); }

	private:
		virtual void OnErrorChild() const override;

		void ResetImpl(dispatch_tag<PositionerStage>) override final;
		virtual void ResetImpl(dispatch_tag<NP_Conex_CC>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NP_Conex_CC_Tasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NP_Conex_CC_Tasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NP_Conex_CC_Tasks::UpdateTask>(); }
	};
}