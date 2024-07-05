// This file is part of DynExp.

/**
 * @file Stage.h
 * @brief Implementation of a meta instrument to control single-axis positioner stages.
*/

#pragma once

#include "stdafx.h"
#include "Instrument.h"

namespace DynExpInstr
{
	class PositionerStage;
	
	/**
	 * @brief Tasks for @p PositionerStage
	*/
	namespace PositionerStageTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		*/
		class InitTask : public DynExp::InitTaskBase
		{
			void InitFuncImpl(dispatch_tag<InitTaskBase>, DynExp::InstrumentInstance& Instance) override final { InitFuncImpl(dispatch_tag<InitTask>(), Instance); }

			/**
			 * @copydoc InitFuncImpl(dispatch_tag<DynExp::InitTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::ExitTaskBase
		*/
		class ExitTask : public DynExp::ExitTaskBase
		{
			void ExitFuncImpl(dispatch_tag<ExitTaskBase>, DynExp::InstrumentInstance& Instance) override final { ExitFuncImpl(dispatch_tag<ExitTask>(), Instance); }

			/**
			 * @copydoc ExitFuncImpl(dispatch_tag<DynExp::ExitTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::UpdateTaskBase
		*/
		class UpdateTask : public DynExp::UpdateTaskBase
		{
			void UpdateFuncImpl(dispatch_tag<UpdateTaskBase>, DynExp::InstrumentInstance& Instance) override final { UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance); }

			/**
			 * @copydoc UpdateFuncImpl(dispatch_tag<DynExp::UpdateTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};
	}

	/**
	 * @brief Data class for @p PositionerStage
	*/
	class PositionerStageData : public DynExp::InstrumentDataBase
	{
	public:
		/**
		 * @brief Numeric type to store the stage positions.
		*/
		using PositionType = signed long long;

		PositionerStageData() = default;
		virtual ~PositionerStageData() = default;

		auto GetCurrentPosition() const noexcept { return Position; }							//!< Returns #Position.
		auto GetVelocity() const noexcept { return Velocity; }									//!< Returns #Velocity.
		void SetCurrentPosition(PositionType Position) noexcept { this->Position = Position; }	//!< Sets #Position to @p #Position.
		void SetVelocity(PositionType Velocity) noexcept { this->Velocity = Velocity; }			//!< Sets #Velocity to @p #Velocity.

		bool IsMoving() const noexcept { return IsMovingChild(); }								//!< Returns whether the stage is currently moving (result of @p IsMovingChild())
		bool HasArrived() const noexcept { return HasArrivedChild(); }							//!< Returns whether the stage has arrived at its destiny position (result of @p HasArrivedChild())
		bool HasFailed() const noexcept { return HasFailedChild(); }							//!< Returns whether the stage is in an error state, i.e. moving has failed (result of @p HasFailedChild())
		bool IsReferenced() const noexcept { return IsReferencedChild(); }						//!< Returns whether a closed-loop positioner knows its position in respect to its zero point (result of @p IsReferencedChild())

	private:
		void ResetImpl(dispatch_tag<InstrumentDataBase>) override final;

		/**
		 * @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>)
		*/
		virtual void ResetImpl(dispatch_tag<PositionerStageData>) {};

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual bool IsMovingChild() const noexcept = 0;										//!< @copydoc IsMoving
		virtual bool HasArrivedChild() const noexcept = 0;										//!< @copydoc HasArrived
		virtual bool HasFailedChild() const noexcept = 0;										//!< @copydoc HasFailed
		virtual bool IsReferencedChild() const noexcept { return false; }						//!< @copydoc IsReferenced
		///@}

		/**
		 * @brief Position in nm if the respective stage supports SI units. Otherwise, in units of steps performed.
		*/
		PositionType Position = 0;

		/**
		 * @brief Velocity in nm/s if the respective stage supports SI units. Otherwise, in units of steps/s.
		*/
		PositionType Velocity = 0;
	};

	/**
	 * @brief Parameter class for @p PositionerStage
	*/
	class PositionerStageParams : public DynExp::InstrumentParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p PositionerStage instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		PositionerStageParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : InstrumentParamsBase(ID, Core) {}

		virtual ~PositionerStageParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "PositionerStageParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<InstrumentParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<PositionerStageParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<PositionerStageParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::InstrumentParamsBase>)

		DummyParam Dummy = { *this };												//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p PositionerStage
	*/
	class PositionerStageConfigurator : public DynExp::InstrumentConfiguratorBase
	{
	public:
		using ObjectType = PositionerStage;
		using ParamsType = PositionerStageParams;

		PositionerStageConfigurator() = default;
		virtual ~PositionerStageConfigurator() = 0;
	};

	/**
	 * @brief Implementation of a meta instrument to control single-axis positioner stages.
	*/
	class PositionerStage : public DynExp::InstrumentBase
	{
	public:
		/**
		 * @brief Type to determine the direction of the positioner stage's movements
		*/
		enum class DirectionType { Forward, Backward };
		/**
		 * @var PositionerStage::DirectionType PositionerStage::Forward
		 * The position is supposed to move in forward direction.
		*/
		/**
		 * @var PositionerStage::DirectionType PositionerStage::Backward
		 * The position is supposed to move in backward direction.
		*/

		using ParamsType = PositionerStageParams;								//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = PositionerStageConfigurator;							//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = PositionerStageData;							//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = PositionerStageTasks::InitTask;					//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = PositionerStageTasks::ExitTask;					//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = PositionerStageTasks::UpdateTask;				//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Positioner Stage"; }	//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "Positioners"; }		//!< @copydoc DynExp::InstrumentBase::Category
		
		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		PositionerStage(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: InstrumentBase(OwnerThreadID, std::move(Params)) {}

		virtual ~PositionerStage() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(100); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		*/
		///@{
		virtual PositionerStageData::PositionType GetMinPosition() const noexcept = 0;		//!< Returns the minimal position the stage can move to in nm if the stage supports SI units, in step units otherwise.
		virtual PositionerStageData::PositionType GetMaxPosition() const noexcept = 0;		//!< Returns the maximal position the stage can move to in nm if the stage supports SI units, in step units otherwise.
		virtual PositionerStageData::PositionType GetResolution() const noexcept = 0;		//!< Returns the stage's position resolution (precision) in nm if the stage supports SI units, in step units otherwise.
		
		virtual PositionerStageData::PositionType GetMinVelocity() const noexcept = 0;		//!< Returns the minimal velocity the stage can move with in nm/s if the stage supports SI units, in steps/s otherwise.
		virtual PositionerStageData::PositionType GetMaxVelocity() const noexcept = 0;		//!< Returns the maximal velocity the stage can move with in nm/s if the stage supports SI units, in steps/s otherwise.
		virtual PositionerStageData::PositionType GetDefaultVelocity() const noexcept = 0;	//!< Returns the stage's default velocity in nm/s if the stage supports SI units, in steps/s otherwise.

		/**
		 * @brief Determines the conversion factor in between internal PositionerStageData::PositionType
		 * positiond and velocities and the units the underlying hardware expects.
		 * @return Returns a factor positions and velocities are divided by before sending the values to
		 * the hardware. This is useful if the underlying hardware e.g. operates in mm instead of nm. 
		*/
		virtual double GetStepNanoMeterRatio() const noexcept { return 1; }

		/**
		 * @brief Determines whether the underlying hardware expects SI units for positions
		 * and velocities or arbitrary units.
		 * @return Return true if the underlying hardware deals with SI units, false otherwise.
		*/
		virtual bool IsUsingSIUnits() const noexcept { return false; }
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		/**
		 * @brief Stores the positioner's home position. Also refer to @p MoveToHome().
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetHome() const;

		/**
		 * @brief References the positioner such that it finds its zero position or end stop.
		 * @param Direction Direction to move into to find the reference mark. This argument
		 * might be ignored.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void Reference(DirectionType Direction = DirectionType::Forward, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Calibrates the positioner. The meaning of such a calibration is solely specified
		 * by the hardware manufacturer of the respective positioner type.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void Calibrate(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Sets the positioner's velocity in its native units. Do divide units of
		 * PositionerStageData::Velocity by @p GetStepNanoMeterRatio() to obtain native units.
		 * Also refer to @p IsUsingSIUnits().
		 * @param Velocity Velocity in the positioner's native units
		*/
		virtual void SetVelocity(const PositionerStageData::PositionType Velocity) const = 0;

		/**
		 * @brief Moves the positioner to its stored home position. Also refer to @p SetHome().
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void MoveToHome(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Moves the positioner to an absolute position in its native units. Do divide
		 * units of PositionerStageData::Position by @p GetStepNanoMeterRatio() to obtain
		 * native units. Also refer to @p IsUsingSIUnits().
		 * @param Steps Absolute position in the positioner's native units
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void MoveAbsolute(const PositionerStageData::PositionType Steps, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Moves the positioner to a position relative to its current position. The
		 * position offset @p Steps has to be specified in native units. Do divide
		 * units of PositionerStageData::Position by @p GetStepNanoMeterRatio() to obtain
		 * native units. Also refer to @p IsUsingSIUnits().
		 * @param Steps Relative position in the positioner's native units
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void MoveRelative(const PositionerStageData::PositionType Steps, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Stops any motion or position stabilization of the positioner immediately.
		*/
		virtual void StopMotion() const = 0;
		///@}

	private:
		void ResetImpl(dispatch_tag<InstrumentBase>) override final;
		virtual void ResetImpl(dispatch_tag<PositionerStage>) = 0;				//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentBase>)
	};
}