// This file is part of DynExp.

/**
 * @file Laser.h
 * @brief Defines a meta instrument for a laser source.
*/

#pragma once

#include "stdafx.h"
#include "Instrument.h"

namespace DynExpInstr
{
	class Laser;

	/**
	 * @brief Tasks for @p Laser
	*/
	namespace LaserTasks
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
	 * @brief Data class for @p Laser
	*/
	class LaserData : public DynExp::InstrumentDataBase
	{
	public:
		/**
		 * @brief Possible laser states.
		*/
		enum class LaserStateType {
			Startup,					//!< The laser is warming up.
			Ready,						//!< The laser is ready for emission.
			EmissionEnabledConstant,	//!< The laser is emitting in constant mode.
			EmissionEnabledScanning,	//!< The laser is emitting in scan mode.
			Error,						//!< The laser is in an error state.
		};

		LaserData() = default;
		virtual ~LaserData() = default;

		void SetFrequencyValue(double Frequency) noexcept { this->Frequency = Frequency; }				//!< Setter for #Frequency.
		auto GetFrequencyValue() const noexcept { return Frequency; }									//!< Getter for #Frequency.
		void SetIntensityValue(double Intensity) noexcept { this->Intensity = Intensity; }				//!< Setter for #Intensity.
		auto GetIntensityValue() const noexcept { return Intensity; }									//!< Getter for #Intensity.
		void SetScanRangeValue(double ScanRange) noexcept { this->ScanRange = ScanRange; }				//!< Setter for #ScanRange.
		auto GetScanRangeValue() const noexcept { return ScanRange; }									//!< Getter for #ScanRange.
		void SetScanRateValue(double ScanRate) noexcept { this->ScanRate = ScanRate; }					//!< Setter for #ScanRate.
		auto GetScanRateValue() const noexcept { return ScanRate; }										//!< Getter for #ScanRate.

		/**
		 * @brief Returns the laser's current state.
		 * @return Laser state of type LaserData::StateType
		*/
		auto GetLaserState() const noexcept { return GetLaserStateChild(); }
		
	private:
		void ResetImpl(dispatch_tag<InstrumentDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<LaserData>) {};					//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>)

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual LaserStateType GetLaserStateChild() const noexcept = 0;		//!< @copydoc GetLaserState
		///@}

		double Frequency = 0.0;				//!< Current frequency 
		double Intensity = 0.0;				//!< Current intensity
		double ScanRange = 0.0;				//!< Current scan range
		double ScanRate = 0.0;				//!< Current scan rate
	};

	/**
	 * @brief Parameter class for @p Laser.
	*/
	class LaserParams : public DynExp::InstrumentParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p Laser instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		LaserParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : InstrumentParamsBase(ID, Core) {}

		virtual ~LaserParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "LaserParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<InstrumentParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<LaserParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<LaserParams>) {}			//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::InstrumentParamsBase>)

		DummyParam Dummy = { *this };											//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p Laser
	*/
	class LaserConfigurator : public DynExp::InstrumentConfiguratorBase
	{
	public:
		using ObjectType = Laser;
		using ParamsType = LaserParams;

		LaserConfigurator() = default;
		virtual ~LaserConfigurator() = 0;
	};

	/**
	 * @brief Meta instrument for a laser.
	*/
	class Laser : public DynExp::InstrumentBase
	{
	public:
		using ParamsType = LaserParams;										//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = LaserConfigurator;								//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = LaserData;								//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = LaserTasks::InitTask;							//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = LaserTasks::ExitTask;							//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = LaserTasks::UpdateTask;						//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Laser"; }			//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }			//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		Laser(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: InstrumentBase(OwnerThreadID, std::move(Params)) {}

		virtual ~Laser() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(50); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		*/
		///@{
		/**
		 * @brief Determines the frequency unit.
		 * @return Frequency unit
		*/
		virtual DynExp::Units::UnitType GetFrequencyUnit() const = 0;
		
		/**
		 * @brief Determines the intensity unit.
		 * @return Intensity unit
		*/
		virtual DynExp::Units::UnitType GetIntensityUnit() const = 0;

		/**
		 * @brief Determines the minimal emission frequency. The default implementation returns
		 * @p GetMaxFrequency() which indicates that the laser does not support frequency tuning.
		 * @return Minimal emission frequency in units of @p GetFrequencyUnit().
		*/
		virtual double GetMinFrequency() const { return GetMaxFrequency(); }

		/**
		 * @brief Determines the maximal emission frequency.
		 * @return Maximal emission frequency in units of @p GetFrequencyUnit().
		*/
		virtual double GetMaxFrequency() const = 0;

		/**
		 * @brief Determines the minimal emission intensity. The default implementation returns
		 * @p GetMaxIntensity() which indicates that the laser does not support intensity adjustment.
		 * @return Minimal emission intensity in units of @p GetIntensityUnit().
		*/
		virtual double GetMinIntensity() const { return GetMaxIntensity(); }

		/**
		 * @brief Determines the maximal emission intensity.
		 * @return Maximal emission intensity in units of @p GetIntensityUnit().
		*/
		virtual double GetMaxIntensity() const = 0;

		/**
		 * @brief Determines the minimal frequency scan range.
		 * The default implementation returns 0.0, indicating that the laser does not support scanning.
		 * @return Minimal scan range in units of @p GetFrequencyUnit().
		*/
		virtual double GetMinScanRange() const { return 0.0; }

		/**
		 * @brief Determines the maximal frequency scan range.
		 * The default implementation returns 0.0, indicating that the laser does not support scanning.
		 * @return Maximal scan range in units of @p GetFrequencyUnit().
		*/
		virtual double GetMaxScanRange() const { return 0.0; }

		/**
		 * @brief Determines the minimal frequency scan rate.
		 * The default implementation returns 0.0, indicating that the laser does not support scanning.
		 * @return Minimal scan rate in units of @p GetFrequencyUnit() per second.
		*/
		virtual double GetMinScanRate() const { return 0.0; }

		/**
		 * @brief Determines the maximal frequency scan rate.
		 * The default implementation returns 0.0, indicating that the laser does not support scanning.
		 * @return Maximal scan rate in units of @p GetFrequencyUnit() per second.
		*/
		virtual double GetMaxScanRate() const { return 0.0; }

		/**
		 * @brief Determines the mode hop free tuning range.
		 * The default implementation returns 0.0, indicating that the laser does not support frequency tuning.
		 * @return Mode hop free tuning range in units of @p GetFrequencyUnit().
		*/
		virtual double GetModeHopFreeTuningRange() const { return 0.0; }
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		/**
		 * @brief Sets the laser's emission frequency.
		 * @param Frequency Emission frequency in units of @p GetFrequencyUnit()
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetFrequency(double Frequency, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Sets the laser's output intensity.
		 * @param Intensity Output intensity in units of @p GetIntensityUnit()
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetIntensity(double Intensity, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Sets the laser's frequency scan range.
		 * @param ScanRange Scan range in units of @p GetFrequencyUnit()
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetScanRange(double ScanRange, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Sets the laser's frequency scan rate.
		 * @param ScanRate Scan rate in units of @p GetFrequencyUnit()
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetScanRate(double ScanRate, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Enables emission in constant mode.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Enable(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Disables emission.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Disable(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Enables emission in scan mode.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void ScanContinuously(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Disables scan and changes to constant emission mode.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void DisableScan(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;
		///@}

	private:
		void ResetImpl(dispatch_tag<InstrumentBase>) override final;
		virtual void ResetImpl(dispatch_tag<Laser>) = 0;			//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentBase>)
	};
}