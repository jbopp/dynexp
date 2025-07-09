// This file is part of DynExp.

/**
 * @file Laser.h
 * @brief Defines a meta instrument for a laser.
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
		 * @brief Supported lasers frequency units.
		 * @warning If this is changed, also change @p ConvertFrequencyUnitType() functions in
		 * @p NetworkLaser.h and @p FrequencyUnitType enumeration in @p Common.proto.
		*/
		enum class FrequencyUnitType {
			Hz,			//!< Frequency in Hz
			nm,			//!< Wavelength in nm
			Inv_cm		//!< Wavenumber in 1/cm
		};

		/**
		 * @brief Supported laser intensity units.
		 * @warning If this is changed, also change @p ConvertIntensityUnitType() functions in
		 * @p NetworkLaser.h and @p IntensityUnitType enumeration in @p Common.proto.
		*/
		enum class IntensityUnitType {
			Power_W		//!< Intensity in Watts
		};
		
		/**
		 * @brief Returns a descriptive string of a respective frequency unit to be e.g. used in plots.
		 * @param Unit Frequency unit type as used by spectrometer instruments.
		 * @return Unit string
		*/
		static const char* FrequencyUnitTypeToStr(const FrequencyUnitType& Unit);

		/**
		 * @brief Returns a descriptive string of a respective intensity unit to be e.g. used in plots.
		 * @param Unit Intensity unit type as used by spectrometer instruments.
		 * @return Unit string
		*/
		static const char* IntensityUnitTypeToStr(const IntensityUnitType& Unit);

		/**
		 * @brief Possible laser states.
		*/
		enum class LaserStateType {
			Startup,
			Ready,						//!< The wavelength is set and the laser is ready for emission.
			EmissionEnabledConstant,	//!< The laser is emitting in controle mode.
			EmissionEnabledScanning,	//!< The laser is emitting in scan mode.
			Error,						//!< The laser is in an error state.
		};

		LaserData() = default;
		virtual ~LaserData() = default;

		//void SetFrequency(double CurrentFrequency) noexcept { this->CurrentFrequency = CurrentFrequency; }			//!< Setter for #Frequency.
		//void SetIntensity(double CurrentIntensity) noexcept { this->CurrentIntensity = CurrentIntensity; }			//!< Setter for #Intensity.
		auto GetFrequency() const noexcept { return CurrentFrequency; }												//!< Getter for #Frequency.
		auto GetIntensity() const noexcept { return CurrentIntensity; }												//!< Getter for #Intensity.
		//void SetScanRange(double ScanRange) noexcept { this->ScanRange = ScanRange; }								//!< Setter for #ScanRange.
		//void SetScanRate(double ScanRate) noexcept { this->ScanRate = ScanRate; }									//!< Setter for #ScanRate.
		auto GetScanRange() const noexcept { return ScanRange; }													//!< Getter for #ScanRange.
		auto GetScanRate() const noexcept { return ScanRate; }														//!< Getter for #ScanRate.

		/**
		 * @brief Returns the laser's current state.
		 * @return State of type LaserData::StateType
		*/
		auto GetState() const noexcept { return GetLaserStateChild(); }

		/**
			* @brief Determines whether the laser is currently in emission state.
			* @return Returns true if @p GetEmissionState() returns
			* LaserStateType::EmissionEnabledConstant or LaserStateType::EmissionEnabledScanning, false otherwise.
		*/
		bool IsLasing() const noexcept { return GetLaserStateChild() == LaserStateType::EmissionEnabledConstant || GetLaserStateChild() == LaserStateType::EmissionEnabledScanning; }

		
	private:
		
		void ResetImpl(dispatch_tag<InstrumentDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<LaserData>) {};					//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>)

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual LaserStateType GetLaserStateChild() const noexcept = 0;		//!< @copydoc GetLaserState
		///@}

		double CurrentFrequency = 0.0;		//!< Current frequency measured by WLM
		double CurrentIntensity = 0.0;		//!< Current intensity at SHG output
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
		virtual void ConfigureParamsImpl(dispatch_tag<LaserParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::InstrumentParamsBase>)

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
		constexpr static auto Category() noexcept { return "Laser"; }		//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		Laser(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: InstrumentBase(OwnerThreadID, std::move(Params)) {
		}

		virtual ~Laser() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		/** @name Override (instrument information)
			* Override by derived classes to provide information about the instrument.
		*/
		///@{
		/**
			* @brief Determines the frequency unit.
			* @return Frequency unit
		*/
		virtual LaserData::FrequencyUnitType GetFrequencyUnit() const = 0;
		
		/**
			* @brief Determines the intensity unit.
			* @return Intensity unit
		*/
		virtual LaserData::IntensityUnitType GetIntensityUnit() const = 0;

		/**
		 * @brief Determines the minimal lower frequency limit of emission.
		 * @return Minimal lower frequency limit in units of @p GetFrequencyUnit().
		*/
		virtual double GetMinFrequency() const = 0;

		/**
		 * @brief Determines the maximal upper frequency limit of emission.
		 * @return Maximal upper frequency limit in units of @p GetFrequencyUnit().
		*/
		virtual double GetMaxFrequency() const = 0;

		/**
		 * @brief Determines the minimal emission intensity.
		 * @return Minimal emission intensity in units of @p GetIntensityUnit().
		*/
		virtual double GetMinIntensity() const = 0;

		/**
		 * @brief Determines the maximal emission intensity.
		 * @return Maximal emission intensity in units of @p GetIntensityUnit().
		*/
		virtual double GetMaxIntensity() const = 0;

		/**
		 * @brief Determines the minimal scan bandwidth.
		 * @return Minimal bandwidth in units of @p GetFrequencyUnit().
		*/
		virtual double GetMinBandwidth() const = 0;

		/**
		 * @brief Determines the maximal scan bandwidth.
		 * @return Maximal scan bandwidth in units of @p GetFrequencyUnit().
		*/
		virtual double GetMaxBandwidth() const = 0;

		/**
		 * @brief Determines the maximal scan rate.
		 * @return Maximal scan rate in units of @p GetFrequencyUnit() per second.
		*/
		virtual double GetMaxRate() const = 0;
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		/**
		 * @brief Sets the laser's dial frequency.
		 * @param Frequency Frequency to dial
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetFrequency(double Frequency, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Sets the laser's intensity.
		 * @param Intensity Output intensity
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetIntensity(double Intensity, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Sets the laser's scan range.
		 * @param Bandwidth Bandwidth for scan
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetScanRange(double ScanRange, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Sets the laser's scan rate.
		 * @param Rate Rate of scan
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetScanRate(double ScanRate, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Enables emission.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Enable(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Disables emission.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Disable(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Starts scan.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void ScanContinuously(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Stops scan.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void DisableScan(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;
		///@}

	private:
		void ResetImpl(dispatch_tag<InstrumentBase>) override final;
		virtual void ResetImpl(dispatch_tag<Laser>) = 0;			//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentBase>)
	};
}