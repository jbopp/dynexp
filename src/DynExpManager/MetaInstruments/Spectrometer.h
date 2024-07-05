// This file is part of DynExp.

/**
 * @file Spectrometer.h
 * @brief Defines a meta instrument for a spectrometer.
*/

#pragma once

#include "stdafx.h"
#include "Instrument.h"

namespace DynExpInstr
{
	class Spectrometer;

	/**
	 * @brief Tasks for @p Spectrometer
	*/
	namespace SpectrometerTasks
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
	 * @brief Data class for @p Spectrometer
	*/
	class SpectrometerData : public DynExp::InstrumentDataBase
	{
	public:
		/**
		 * @brief Time type describing the spectrometer's times like its exposure time
		*/
		using TimeType = std::chrono::milliseconds;

		/**
		 * @brief Supported spectrometer frequency units.
		 * @warning If this is changed, also change @p ConvertFrequencyUnitType() functions in
		 * @p NetworkSpectrometer.h and @p FrequencyUnitType enumeration in @p Common.proto.
		*/
		enum class FrequencyUnitType {
			Hz,			//!< Frequency in Hz
			nm,			//!< Wavelength in nm
			Inv_cm		//!< Wavenumber in 1/cm
		};

		/**
		 * @brief Supported spectrometer intensity units.
		 * @warning If this is changed, also change @p ConvertIntensityUnitType() functions in
		 * @p NetworkSpectrometer.h and @p IntensityUnitType enumeration in @p Common.proto.
		*/
		enum class IntensityUnitType {
			Counts		//!< Number of counts (arbitrary unit)
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
		 * @brief Possible spectrometer states.
		*/
		enum class CapturingStateType {
			Ready,		//!< The spectrometer is ready to acquire a spectrum.
			Warning,	//!< The spectrometer is in a warning state, but still ready to acquire a spectrum.
			Error,		//!< The spectrometer is in an error state.
			Capturing	//!< The spectrometer is currently acquiring a spectrum.
		};

		/**
		 * @brief Type describing a spectrum as acquired by the @p Spectrometer instrument.
		*/
		class SpectrumType
		{
		public:
			/**
			 * @brief Constructs a @p SpectrumType instance with #FrequencyUnit set to
			 * FrequencyUnitType::Hz and #IntensityUnit set to IntensityUnitType::Counts.
			*/
			SpectrumType() : FrequencyUnit(FrequencyUnitType::Hz), IntensityUnit(IntensityUnitType::Counts) {}
			
			/**
			 * @brief Constructs a @p SpectrumType instance with the specified units.
			 * @param FrequencyUnit @copybrief #FrequencyUnit
			 * @param IntensityUnit @copybrief #IntensityUnit
			*/
			SpectrumType(FrequencyUnitType FrequencyUnit, IntensityUnitType IntensityUnit) : FrequencyUnit(FrequencyUnit), IntensityUnit(IntensityUnit) {}
			
			/**
			 * @brief Copy-constructs a @p SpectrumType instance.
			 * @param Other Spectrum to copy from
			*/
			SpectrumType(const SpectrumType& Other) : FrequencyUnit(Other.FrequencyUnit), IntensityUnit(Other.IntensityUnit), Samples(Other.Samples) {}
			
			/**
			 * @brief Move-constructs a @p SpectrumType instance.
			 * @param Other Spectrum to move from. #Samples of @p Other will be empty after the operation.
			*/
			SpectrumType(SpectrumType&& Other);

			/**
			 * @brief Copies a @p SpectrumType instance's content to this instance.
			 * @param Other Spectrum to copy from
			 * @return Returns a reference to this @p SpectrumType instance.
			*/
			SpectrumType& operator=(const SpectrumType& Other);

			/**
			 * @brief Moves a @p SpectrumType instance's content to this instance.
			 * @param Other Spectrum to move from. #Samples of @p Other will be empty after the operation.
			 * @return Returns a reference to this @p SpectrumType instance.
			*/
			SpectrumType& operator=(SpectrumType&& Other);

			void Reset();	//!< Removes all samples from the spectrum (clears #Samples).

			auto GetFrequencyUnit() const noexcept { return FrequencyUnit; }	//!< Getter for #FrequencyUnit.
			auto GetIntensityUnit() const noexcept { return IntensityUnit; }	//!< Getter for #IntensityUnit.
			auto& GetSpectrum() const noexcept { return Samples; }				//!< Getter for #Samples.
			auto& GetSpectrum() noexcept { return Samples; }					//!< Getter for #Samples.

			/**
			 * @brief Indicates whether the spectrum is empty.
			 * @return Returns true when SpectrumType::Samples contain at least one sample, false otherwise.
			*/
			bool HasSpectrum() const noexcept { return !Samples.empty(); }

		private:
			FrequencyUnitType FrequencyUnit;	//!< The spectrum's frequency (x-axis) unit.
			IntensityUnitType IntensityUnit;	//!< The spectrum's intensity (y-axis) unit.

			std::map<double, double> Samples;	//!< Samples of the spectrum as tuples in units (#FrequencyUnit, #IntensityUnit)
		};

		SpectrometerData() = default;
		virtual ~SpectrometerData() = default;

		auto GetMinExposureTime() const noexcept { return MinExposureTime; }																				//!< Getter for #MinExposureTime.
		void SetMinExposureTime(double MinExposureTime) noexcept { this->MinExposureTime = TimeType(Util::NumToT<int>(MinExposureTime)); }					//!< Setter for #MinExposureTime.
		void SetMinExposureTime(TimeType MinExposureTime) noexcept { this->MinExposureTime = MinExposureTime; }												//!< Setter for #MinExposureTime.
		auto GetMaxExposureTime() const noexcept { return MaxExposureTime; }																				//!< Getter for #MaxExposureTime.
		void SetMaxExposureTime(double MaxExposureTime) noexcept { this->MaxExposureTime = TimeType(Util::NumToT<int>(MaxExposureTime)); }					//!< Setter for #MaxExposureTime.
		void SetMaxExposureTime(TimeType MaxExposureTime) noexcept { this->MaxExposureTime = MaxExposureTime; }												//!< Setter for #MaxExposureTime.
		auto GetCurrentExposureTime() const noexcept { return CurrentExposureTime; }																		//!< Getter for #CurrentExposureTime.
		void SetCurrentExposureTime(double CurrentExposureTime) noexcept { this->CurrentExposureTime = TimeType(Util::NumToT<int>(CurrentExposureTime)); }	//!< Setter for #CurrentExposureTime.
		void SetCurrentExposureTime(TimeType CurrentExposureTime) noexcept { this->CurrentExposureTime = CurrentExposureTime; }								//!< Setter for #CurrentExposureTime.
		auto GetCurrentLowerFrequency() const noexcept { return CurrentLowerFrequency; }																	//!< Getter for #CurrentLowerFrequency.
		void SetCurrentLowerFrequency(double CurrentLowerFrequency) noexcept { this->CurrentLowerFrequency = CurrentLowerFrequency; }						//!< Setter for #CurrentLowerFrequency.
		auto GetCurrentUpperFrequency() const noexcept { return CurrentUpperFrequency; }																	//!< Getter for #CurrentUpperFrequency.
		void SetCurrentUpperFrequency(double CurrentUpperFrequency) noexcept { this->CurrentUpperFrequency = CurrentUpperFrequency; }						//!< Setter for #CurrentUpperFrequency.
		auto GetSilentModeEnabled() const noexcept { return SilentModeEnabled; }																			//!< Getter for #SilentModeEnabled.
		void SetSilentModeEnabled(bool Enable) noexcept { SilentModeEnabled = Enable; }																		//!< Setter for #SilentModeEnabled.
		
		/**
		 * @brief Returns the spectrometer's current capturing state.
		 * @return Capturing state of type SpectrometerData::CapturingStateType
		*/
		auto GetCapturingState() const noexcept { return GetCapturingStateChild(); }

		/**
		 * @brief Determines whether the spectrometer is currently acquiring a spectrum.
		 * @return Returns true if @p GetCapturingState() returns
		 * CapturingStateType::Capturing, false otherwise.
		*/
		bool IsCapturing() const noexcept { return GetCapturingStateChild() == CapturingStateType::Capturing; }

		/**
		 * @brief Determines the progress of a spectrum acquisition.
		 * @return Returns the current progress of the spectrum acquisition as a value
		 * in between 0 and 100 (% units).
		*/
		auto GetCapturingProgress() const noexcept { return GetCapturingProgressChild(); }
		
		/**
		 * @copydoc SpectrumType::HasSpectrum
		*/
		bool HasSpectrum() const noexcept { return CurrentSpectrum.HasSpectrum(); }

		/**
		 * @brief Moving getter for #CurrentSpectrum.
		 * @return Returns a spectrum move-constructed from #CurrentSpectrum. Subsequent calls to
		 * @p GetSpectrum() will return an empty spectrum until a new one has been acquired.
		*/
		SpectrumType GetSpectrum() const;

		/**
		 * @brief Copying getter for #CurrentSpectrum. This function is more expensive than
		 * @p GetSpectrum().
		 * @return Returns a spectrum copy-constructed from #CurrentSpectrum.
		*/
		SpectrumType GetSpectrumCopy() const;

		/**
		 * @brief Setter for #CurrentSpectrum.
		 * @param Other Spectrum to set #CurrentSpectrum to by moving from @p Other.
		*/
		void SetSpectrum(SpectrumType&& Other);

		/**
		 * @brief Resets #CurrentSpectrum by calling SpectrumType::Reset().
		*/
		void ClearSpectrum() const;

	private:
		void ResetImpl(dispatch_tag<InstrumentDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<SpectrometerData>) {};					//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>)

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual CapturingStateType GetCapturingStateChild() const noexcept = 0;		//!< @copydoc GetCapturingState
		virtual double GetCapturingProgressChild() const noexcept { return 0; }		//!< @copydoc GetCapturingProgress
		///@}

		TimeType MinExposureTime;				//!< Minimal exposure time the spectrometer supports
		TimeType MaxExposureTime;				//!< Maximal exposure time the spectrometer supports
		TimeType CurrentExposureTime;			//!< Current exposure time of the spectrometer
		double CurrentLowerFrequency = 0.0;		//!< Current lower frequency limit where the spectrum acquisition begins
		double CurrentUpperFrequency = 0.0;		//!< Current upper frequency limit where the spectrum acquisition ends
		bool SilentModeEnabled = false;			//!< Indicates whether the spectrometer's silent mode is turned on, i.e. the spectrometer's fans are turned off.

		/**
		 * @brief Current spectrum acquired by the spectrometer.
		 * Logical const-ness: allow const member function @p GetSpectrum() to move from
		 * #CurrentSpectrum and @p ClearSpectrum() to clear it.
		*/
		mutable SpectrumType CurrentSpectrum;
	};

	/**
	 * @brief Parameter class for @p Spectrometer.
	*/
	class SpectrometerParams : public DynExp::InstrumentParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p Spectrometer instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		SpectrometerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : InstrumentParamsBase(ID, Core) {}

		virtual ~SpectrometerParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "SpectrometerParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<InstrumentParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<SpectrometerParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<SpectrometerParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::InstrumentParamsBase>)

		DummyParam Dummy = { *this };											//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p Spectrometer
	*/
	class SpectrometerConfigurator : public DynExp::InstrumentConfiguratorBase
	{
	public:
		using ObjectType = Spectrometer;
		using ParamsType = SpectrometerParams;

		SpectrometerConfigurator() = default;
		virtual ~SpectrometerConfigurator() = 0;
	};

	/**
	 * @brief Meta instrument for a spectrometer.
	*/
	class Spectrometer : public DynExp::InstrumentBase
	{
	public:
		using ParamsType = SpectrometerParams;										//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = SpectrometerConfigurator;								//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = SpectrometerData;								//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = SpectrometerTasks::InitTask;							//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = SpectrometerTasks::ExitTask;							//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = SpectrometerTasks::UpdateTask;						//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Spectrometer"; }			//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "Image Capturing"; }		//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		Spectrometer(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: InstrumentBase(OwnerThreadID, std::move(Params)) {}

		virtual ~Spectrometer() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }		

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(50); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		*/
		///@{
		/**
		 * @brief Determines the frequency (x-axis) unit of the spectra acquired by the derived instrument.
		 * @return Frequency unit of the acquired spectra
		*/
		virtual SpectrometerData::FrequencyUnitType GetFrequencyUnit() const = 0;

		/**
		 * @brief Determines the intensity (y-axis) unit of the spectra acquired by the derived instrument.
		 * @return Intensity unit of the acquired spectra
		*/
		virtual SpectrometerData::IntensityUnitType GetIntensityUnit() const = 0;

		/**
		 * @brief Determines the minimal lower frequency limit where the spectrum acquisition can begin.
		 * @return Minimal lower frequency limit in units of @p GetFrequencyUnit().
		*/
		virtual double GetMinFrequency() const = 0;

		/**
		 * @brief Determines the maximal upper frequency limit where the spectrum acquisition can end.
		 * @return Maximal upper frequency limit in units of @p GetFrequencyUnit().
		*/
		virtual double GetMaxFrequency() const = 0;
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		/**
		 * @brief Sets the spectrometer's exposure time.
		 * @param ExposureTime Exposure time to be used for spectrum acquisition
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetExposureTime(SpectrometerData::TimeType ExposureTime, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Sets the spectrometer's frequency range where the spectrum acquisition
		 * begins and ends.
		 * @param LowerFrequency Lower frequency limit where the spectrum acquisition begins
		 * @param UpperFrequency Upper frequency limit where the spectrum acquisition ends
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetFrequencyRange(double LowerFrequency, double UpperFrequency, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Enables or disables the spectrometer's silent mode, i.e. disables or
		 * enables e.g. the spectrometer's fans.
		 * The default implementation does not do anything.
		 * @param Enable If true, the silent mode is enabled. If false, it is disabled.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetSilentMode(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Starts the acquisition of a single spectrum.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Record(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Aborts a currently running spectrum acquisition.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void Abort(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;
		///@}

	private:
		void ResetImpl(dispatch_tag<InstrumentBase>) override final;
		virtual void ResetImpl(dispatch_tag<Spectrometer>) = 0;			//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentBase>)
	};
}