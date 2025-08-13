// This file is part of DynExp.

/**
 * @file NP_Conex_CC.h
 * @brief Implementation of an instrument to control Newport stages with the Conex-CC controller.
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

		/**
		 * @brief In this task, some commands cannot be send right after each other to the controller. There has to be some waiting time in between.
		 * Therefore, this tasks calls the subtasks ResetTask, SetHomeExecutionTask and SetReadyTask in this order with a delay in between.
		*/
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

		/**
		 * @brief In this task, some commands cannot be send right after each other to the controller. There has to be some waiting time in between.
		 * Therefore, this tasks calls the subtasks ResetTask, ReferenceExecutionTask and SetReadyTask in this order with a delay in between.
		*/
		class ReferenceTask final : public DynExp::TaskBase
		{
		public:
			ReferenceTask(PositionerStage::DirectionType Direction, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Direction(Direction) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStage::DirectionType Direction;
		};

		class ReferenceExecutionTask final : public DynExp::TaskBase
		{
		public:
			ReferenceExecutionTask(CallbackType CallbackFunc, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
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

		/**
		 * @brief In this task, some commands cannot be send right after each other to the controller. There has to be some waiting time in between.
		 * Therefore, this tasks calls the subtasks StopMotionTask and MoveToHomeExecutionTask in this order with a delay in between.
		*/
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

		/**
		 * @brief In this task, some commands cannot be send right after each other to the controller. There has to be some waiting time in between.
		 * Therefore, this tasks calls the subtasks StopMotionTask and MoveAbsoluteExecutionTask in this order with a delay in between.
		*/
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

		/**
		 * @brief In this task, some commands cannot be send right after each other to the controller. There has to be some waiting time in between.
		 * Therefore, this tasks calls the subtasks StopMotionTask and MoveRelativeExecutionTask in this order with a delay in between.
		*/
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
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class NP_Conex_CC_StageData : public PositionerStageData
	{
		friend class NP_Conex_CC_Tasks::InitTask;
		friend class NP_Conex_CC_Tasks::UpdateTask;

	public:
		using ChannelType = int16_t;

		/**
		 * @brief Interprets and queries the controller's internal state based on the TS command byte code.
		*/
		struct Conex_CCStatusType
		{
			constexpr void Set(uint8_t ByteCode) noexcept { this->ByteCode = ByteCode; }

			/** @name Newport Conex-CC controller states
			 * These methods identify the current internal state of the CONEX-CC motion controller,
			 * based on the `ByteCode` value returned by the `TS` command.
			*/
			///@{
			// NOT REFERENCED STATES
			/// @brief Controller is NOT REFERENCED, entered from RESET. (`ByteCode == 0x0A`)
			constexpr bool NotReferencedFromReset() const noexcept { return ByteCode == 0x0A; }
			/// @brief Controller is NOT REFERENCED, entered from HOMING. (`ByteCode == 0x0B`)
			constexpr bool NotReferencedFromHoming() const noexcept { return ByteCode == 0x0B; }
			/// @brief Controller is NOT REFERENCED, entered from CONFIGURATION. (`ByteCode == 0x0C`)
			constexpr bool NotReferencedFromConfiguration() const noexcept { return ByteCode == 0x0C; }
			/// @brief Controller is NOT REFERENCED, entered from DISABLE. (`ByteCode == 0x0D`)
			constexpr bool NotReferencedFromDisable() const noexcept { return ByteCode == 0x0D; }
			/// @brief Controller is NOT REFERENCED, entered from READY. (`ByteCode == 0x0E`)
			constexpr bool NotReferencedFromReady() const noexcept { return ByteCode == 0x0E; }
			/// @brief Controller is NOT REFERENCED, entered from MOVING. (`ByteCode == 0x0F`)
			constexpr bool NotReferencedFromMoving() const noexcept { return ByteCode == 0x0F; }
			/// @brief Controller is NOT REFERENCED, with no parameters in memory. (`ByteCode == 0x10`)
			constexpr bool NotReferencedNoParams() const noexcept { return ByteCode == 0x10; }

			// CONFIGURATION
			/// @brief Controller is in CONFIGURATION state. (`ByteCode == 0x14`)
			constexpr bool Configuration() const noexcept { return ByteCode == 0x14; }

			// HOMING
			/// @brief Controller is in HOMING state. (`ByteCode == 0x1E`)
			constexpr bool Homing() const noexcept { return ByteCode == 0x1E; }

			// MOVING
			/// @brief Controller is in MOVING state. (`ByteCode == 0x28`)
			constexpr bool Moving() const noexcept { return ByteCode == 0x28; }

			// READY STATES
			/// @brief Controller is READY, entered from HOMING. (`ByteCode == 0x32`)
			constexpr bool ReadyFromHoming() const noexcept { return ByteCode == 0x32; }
			/// @brief Controller is READY, entered from MOVING. (`ByteCode == 0x33`)
			constexpr bool ReadyFromMoving() const noexcept { return ByteCode == 0x33; }
			/// @brief Controller is READY, entered from DISABLE. (`ByteCode == 0x34`)
			constexpr bool ReadyFromDisable() const noexcept { return ByteCode == 0x34; }

			// READY T STATES
			/// @brief Controller is READY (Tracking), entered from READY. (`ByteCode == 0x36`)
			constexpr bool ReadyTFromReady() const noexcept { return ByteCode == 0x36; }
			/// @brief Controller is READY (Tracking), entered from TRACKING. (`ByteCode == 0x37`)
			constexpr bool ReadyTFromTracking() const noexcept { return ByteCode == 0x37; }
			/// @brief Controller is READY (Tracking), entered from DISABLE T. (`ByteCode == 0x38`)
			constexpr bool ReadyTFromDisableT() const noexcept { return ByteCode == 0x38; }

			// DISABLE STATES
			/// @brief Controller is DISABLED, entered from READY. (`ByteCode == 0x3C`)
			constexpr bool DisableFromReady() const noexcept { return ByteCode == 0x3C; }
			/// @brief Controller is DISABLED, entered from MOVING. (`ByteCode == 0x3D`)
			constexpr bool DisableFromMoving() const noexcept { return ByteCode == 0x3D; }
			/// @brief Controller is DISABLED, entered from TRACKING. (`ByteCode == 0x3E`)
			constexpr bool DisableFromTracking() const noexcept { return ByteCode == 0x3E; }
			/// @brief Controller is DISABLED, entered from READY T. (`ByteCode == 0x3F`)
			constexpr bool DisableFromReadyT() const noexcept { return ByteCode == 0x3F; }

			// TRACKING STATES
			/// @brief Controller is TRACKING, entered from READY T. (`ByteCode == 0x46`)
			constexpr bool TrackingFromReadyT() const noexcept { return ByteCode == 0x46; }
			/// @brief Controller is TRACKING, entered from TRACKING. (`ByteCode == 0x47`)
			constexpr bool TrackingFromTracking() const noexcept { return ByteCode == 0x47; }
			///@}

		private:
			uint8_t ByteCode = 0;
		};

		enum ErrorCodeType : uint16_t {
			NoError,
			OtherError // the exact error needs to be read from the bit flag
		};

		NP_Conex_CC_StageData() = default;
		virtual ~NP_Conex_CC_StageData() = default;

		auto GetChannel() const noexcept { return Channel; }

		auto GetConex_CCStatus() const noexcept { return Conex_CCStatus; }
		auto GetErrorCode() const noexcept { return ErrorCode; }

		DynExp::LinkedObjectWrapperContainer<DynExp::SerialCommunicationHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<PositionerStageData>) override final;
		virtual void ResetImpl(dispatch_tag<NP_Conex_CC_StageData>) {};

		virtual bool IsMovingChild() const noexcept override;
		virtual bool HasArrivedChild() const noexcept override;
		virtual bool HasFailedChild() const noexcept override;

		ChannelType Channel = 0;

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
		Param<ParamsConfigDialog::NumberType> Conex_CC_Address = { *this, "Conex_CC_Address", "Conex-CC address",
			"Address (1-31) of the Conex controller to be used", true, 1, 1, 31 };

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
		using InstrumentDataType = NP_Conex_CC_StageData;

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
		virtual PositionerStageData::PositionType GetDefaultVelocity() const noexcept override { return 10e6; } // The maximum velocity is 1e11 * GetInputValuePositionTypeRatio().
		double GetInputValuePositionTypeRatio() const noexcept { return 1e6; } // the controller expects a float as position with 6 digits of precision

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