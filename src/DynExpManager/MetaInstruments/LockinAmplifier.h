// This file is part of DynExp.

/**
 * @file LockinAmplifier.h
 * @brief Defines a meta instrument for a lock-in amplifier.
*/

#pragma once

#include "stdafx.h"
#include "DataStreamInstrument.h"

namespace DynExpInstr
{
	class LockinAmplifier;

	/**
	 * @brief Definitions for lock-in amplifier instruments.
	*/
	namespace LockinAmplifierDefs
	{
		/**
		 * @brief Type to determine the trigger mode.
		 * Not a strongly-typed enum to allow using the enumeration in a
		 * DynExp::ParamsBase::Param in class DynExpInstr::LockinAmplifierParams.
		*/
		enum TriggerModeType {
			UnknownMode,	//!< Trigger mode not specified
			Continuous,		//!< Run continuously disabling the trigger
			ExternSingle	//!< Run once after an external trigger signal has been detected
		};

		/**
		 * @brief Type to determine at which edge of a trigger signal to trigger.
		 * Not a strongly-typed enum to allow using the enumeration in a
		 * DynExp::ParamsBase::Param in class DynExpInstr::LockinAmplifierParams.
		*/
		enum TriggerEdgeType {
			UnknownEdge,	//!< Trigger edge not specified
			Rise,			//!< Trigger on rising edge
			Fall			//!< Trigger on falling edge
		};

		/**
		 * @brief Type specifying different signal coordinates a lock-in amplifier can record.
		 * Not a strongly-typed enum to allow using the enumeration in a
		 * DynExp::ParamsBase::Param in class DynExpInstr::LockinAmplifierParams.
		*/
		enum SignalType {
			X,				//!< X component of the signal in cartesian coordinates
			Y,				//!< Y component of the signal in cartesian coordinates
			R,				//!< Radial component of the signal in polar coordinates
			Theta			//!< Phase component of the signal in polar coordinates
		};

		/**
		 * @brief Type describing a recorded lock-in sample in cartesian coordinates.
		 * Must be trivially copyable.
		*/
		struct LockinResultCartesian
		{
			using DataType = double;	//!< Data type used to store the sample

			/**
			 * @brief Default-constructs a @p LockinResultCartesian instance.
			*/
			constexpr LockinResultCartesian() noexcept = default;

			/**
			 * @brief Constructs a @p LockinResultCartesian instance with the given coordinates.
			 * @param X @copydoc LockinAmplifierDefs::X
			 * @param Y @copydoc LockinAmplifierDefs::Y
			*/
			constexpr LockinResultCartesian(DataType X, DataType Y) noexcept : X(X), Y(Y) {}

			DataType X{};				//!< @copydoc LockinAmplifierDefs::X
			DataType Y{};				//!< @copydoc LockinAmplifierDefs::Y
		};

		/**
		 * @brief Type describing a recorded lock-in sample in polar coordinates.
		 * Must be trivially copyable.
		*/
		struct LockinResultPolar
		{
			using DataType = double;	//!< Data type used to store the sample

			/**
			 * @brief Default-constructs a @p LockinResultPolar instance.
			*/
			constexpr LockinResultPolar() noexcept = default;

			/**
			 * @brief Constructs a @p LockinResultCartesian instance from the given coordinates.
			 * @param R @copydoc LockinAmplifierDefs::R
			 * @param Theta @copydoc LockinAmplifierDefs::Theta
			*/
			constexpr LockinResultPolar(DataType R, DataType Theta) noexcept : R(R), Theta(Theta) {}

			DataType R{};				//!< @copydoc LockinAmplifierDefs::R
			DataType Theta{};			//!< @copydoc LockinAmplifierDefs::Theta
		};

		/**
		 * @brief Type describing a recorded lock-in sample independent of the coordinate system.
		 * Must be trivially copyable.
		*/
		struct LockinSample
		{
			using DataType = double;	//!< Data type used to store the sample (value and time)

			/**
			 * @brief Default-constructs a @p LockinSample instance.
			*/
			constexpr LockinSample() noexcept = default;

			/**
			 * @brief Constructs a @p LockinSample instance from the given cartesian result representation.
			 * Calls @p UpdatePolar().
			 * @param Channel @copybrief #Channel
			 * @param Time @copybrief #Time
			 * @param CartesianResult @copybrief CartesianResult
			*/
			LockinSample(uint8_t Channel, DataType Time, LockinResultCartesian CartesianResult) noexcept;

			/**
			 * @brief Constructs a @p LockinSample instance from the given polar result representation.
			 * Calls @p UpdateCartesian().
			 * @param Channel @copybrief #Channel
			 * @param Time @copybrief #Time
			 * @param PolarResult @copybrief PolarResult
			*/
			LockinSample(uint8_t Channel, DataType Time, LockinResultPolar PolarResult) noexcept;

			/**
			 * @brief Constructs a @p LockinSample instance from the given cartesian and polar result
			 * representations assuming that both are equal when converted to the respective other
			 * result representation.
			 * @param Channel @copybrief #Channel
			 * @param Time @copybrief #Time
			 * @param CartesianResult @copybrief CartesianResult
			 * @param PolarResult @copybrief PolarResult
			*/
			constexpr LockinSample(uint8_t Channel, DataType Time, LockinResultCartesian CartesianResult, LockinResultPolar PolarResult) noexcept;

			/**
			 * @brief Updates #PolarResult based on #CartesianResult.
			*/
			void UpdatePolar() noexcept;

			/**
			 * @brief Updates #CartesianResult based on #PolarResult.
			*/
			void UpdateCartesian() noexcept;

			/**
			 * @brief Retrieves the sample value represented as the coordinate specified by @p Signal.
			 * @param Signal Coordinate of the sample value to return
			 * @return Returns the requested coordinate of the sample value.
			*/
			DataType GetDisambiguatedValue(SignalType Signal) const noexcept;

			uint8_t Channel{};			//!< Channel of the lock-in amplifier this sample has been recorded with

			LockinResultCartesian CartesianResult{};	//!< Representation of the recorded sample in cartesian coordinates
			LockinResultPolar PolarResult{};			//!< Representation of the recorded sample in polar coordinates

			DataType Time{};			//!< Time in seconds this sample has been recorded at
		};
	}

	/**
	 * @brief Tasks for @p LockinAmplifier
	*/
	namespace LockinAmplifierTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		*/
		class InitTask : public DataStreamInstrumentTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;

			/**
			 * @copydoc InitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::InitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::ExitTaskBase
		*/
		class ExitTask : public DataStreamInstrumentTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final { ExitFuncImpl(dispatch_tag<ExitTask>(), Instance); }

			/**
			 * @copydoc ExitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::ExitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::UpdateTaskBase
		*/
		class UpdateTask : public DataStreamInstrumentTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<DataStreamInstrumentTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final { UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance); }

			/**
			 * @copydoc UpdateFuncImpl(dispatch_tag<DataStreamInstrumentTasks::UpdateTask>, DynExp::InstrumentInstance&)
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};
	}

	/**
	 * @brief Data class for @p LockinAmplifier
	*/
	class LockinAmplifierData : public DataStreamInstrumentData
	{
	public:
		using SampleStreamType = BasicSampleStream;		//!< Data stream type this data stream instrument operates on.

		/**
		 * @brief Constructs a @p LockinAmplifierData instance.
		 * @param BufferSizeInSamples Initial buffer size of a data stream of type
		 * @p SampleStreamType to create for the related @p DataStreamInstrument
		 * instance to operate on
		*/
		LockinAmplifierData(size_t BufferSizeInSamples) : DataStreamInstrumentData(std::make_unique<SampleStreamType>(BufferSizeInSamples)) {}

		virtual ~LockinAmplifierData() = default;

		/** @name Override
		 * Override getters and setters by derived classes to let them decide
		 * which functionality to provide and how to represent the data.
		*/
		///@{
		virtual double GetSensitivity() const noexcept = 0;				//!< Getter for the lock-in amplifier's sensitivity/amplification setting
		virtual void SetSensitivity(double Sensitivity) noexcept = 0;	//!< Setter for the lock-in amplifier's sensitivity/amplification setting
		virtual double GetPhase() const noexcept = 0;					//!< Getter for the phase in rad of the lock-in amplifier's demodulator
		virtual void SetPhase(double Phase) noexcept = 0;				//!< Setter for the phase in rad of the lock-in amplifier's demodulator
		virtual double GetTimeConstant() const noexcept = 0;			//!< Getter for the time constant in seconds of the lock-in amplifier's low-poss filter
		virtual void SetTimeConstant(double TimeConstant) noexcept = 0;	//!< Setter for the time constant in seconds of the lock-in amplifier's low-poss filter
		virtual uint8_t GetFilterOrder() const noexcept { return 0; }	//!< Getter for the filter order/quality of the lock-in amplifier's low-poss filter
		virtual void SetFilterOrder(uint8_t FilterOrder) noexcept {}	//!< Setter for the filter order/quality of the lock-in amplifier's low-poss filter

		/**
		 * @brief Getter for the lock-in amplifier's trigger mode
		*/
		virtual LockinAmplifierDefs::TriggerModeType GetTriggerMode() const noexcept { return LockinAmplifierDefs::TriggerModeType::UnknownMode; }

		/**
		 * @brief Setter for the lock-in amplifier's trigger mode
		*/
		virtual void SetTriggerMode(LockinAmplifierDefs::TriggerModeType TriggerMode) noexcept {}

		/**
		 * @brief Getter for the lock-in amplifier's trigger edge
		*/
		virtual LockinAmplifierDefs::TriggerEdgeType GetTriggerEdge() const noexcept { return LockinAmplifierDefs::TriggerEdgeType::UnknownEdge; }

		/**
		 * @brief Setter for the lock-in amplifier's trigger edge
		*/
		virtual void SetTriggerEdge(LockinAmplifierDefs::TriggerEdgeType TriggerEdge) noexcept {}

		/**
		 * @brief Getter for the lock-in amplifier's signal coordinate type to write to the data stream
		*/
		virtual LockinAmplifierDefs::SignalType GetSignalType() const noexcept = 0;

		/**
		 * @brief Setter for the lock-in amplifier's signal coordinate type to write to the data stream
		*/
		virtual void SetSignalType(LockinAmplifierDefs::SignalType SignalType) noexcept = 0;

		virtual double GetSamplingRate() const noexcept { return 0; }	//!< Getter for the lock-in amplifier's sampling rate in samples/s
		virtual void SetSamplingRate(double SamplingRate) noexcept {}	//!< Setter for the lock-in amplifier's sampling rate in samples/s
		virtual bool IsEnabled() const noexcept { return true; }		//!< Returns true if the lock-in amplifier's demodulator is enabled, false otherwise. 
		virtual void SetEnable(uint8_t Enable) noexcept {}				//!< Enables (pass true) or disables (pass false) the lock-in amplifier's demodulator.

		/**
		 * @brief Indicates whether the lock-in amplifier's physical signal input is overloaded
		 * by e.g. an overvoltage.
		 * @return Returns true if there is currently an overload, false otherwise.
		*/
		virtual bool IsOverloaded() const noexcept { return false; }

		/**
		 * @brief Indicates the load at the lock-in amplifier's physical signal input in
		 * negative and positive directions.
		 * @return Returns a tuple of two values. The first value refers to the load in negative
		 * direction (e.g. negative voltages), the second value refers to the load in positive
		 * direction (e.g. positive voltages). Each value is in range 0 to 1. 1 means an overload
		 * in the respective direction. Values of -1 indicate that the load in the respective
		 * direction is unknown.
		*/
		virtual std::pair<double, double> GetInputLoad() const noexcept { return std::make_pair(-1, -1); }
		
		/**
		 * @brief Indicates the oscillator frequency the lock-in amplifier demodulates the
		 * measured signal at.
		 * @return Returns the oscillator frequency in Hz or 0 if it is unknown.
		*/
		virtual double GetOscillatorFrequency() const noexcept { return 0; }

		/**
		 * @brief Determines the progress of the current data acquisition.
		 * @return Returns the progress of the data acquisition as a value
		 * in between 0 and 1 or -1 if the progress is unknown.
		*/
		virtual double GetAcquisitionProgress() const noexcept { return -1; }
		///@}

		/**
		 * @brief Determines the duration of a single acquisition in seconds using
		 * @p GetSamplingRate() and DataStreamBase::GetStreamSizeWrite().
		 * @return Returns the stream size divided by the sampling rate. Returns 0 if the
		 * sampling rate is not greater than 0.
		*/
		double GetAcquisitionTime() const;

	private:
		void ResetImpl(dispatch_tag<DataStreamInstrumentData>) override final;
		virtual void ResetImpl(dispatch_tag<LockinAmplifierData>) {};			//!< @copydoc ResetImpl(dispatch_tag<DataStreamInstrumentData>)
	};

	/**
	 * @brief Parameter class for @p LockinAmplifier
	*/
	class LockinAmplifierParams : public DataStreamInstrumentParams
	{
	public:
		/**
		 * @brief Type to determine whether to apply parameters stored here to the
		 * lock-in amplifier when the instrument is started.
		*/
		enum AutoApplyParamsType {
			DoNotApply,		//!< Parameters are not applied automatically when the instrument is started.
			AutoApply		//!< Parameters are applied automatically when the instrument is started.
		};

		/**
		 * @brief Maps description strings to the @p AutoApplyParamsType enum's items.
		 * @return List containing the description-value mapping
		*/
		static Util::TextValueListType<AutoApplyParamsType> AutoApplyParamsTypeStrList();

		/**
		 * @brief Constructs the parameters for a @p LockinAmplifier instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		LockinAmplifierParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : DataStreamInstrumentParams(ID, Core) {}

		virtual ~LockinAmplifierParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "LockinAmplifierParams"; }

		/**
		 * @brief Parameter to indicate whether to apply parameters stored here to the
		 * instrument when it is started. Refer to LockinAmplifier::ApplyFromParams().
		*/
		Param<AutoApplyParamsType> AutoApplyParams = { *this, AutoApplyParamsTypeStrList(), "AutoApplyParams", "Auto apply parameters",
			"Determines whether the parameters saved to the instrument are applied automatically on instrument initialization", false, AutoApplyParamsType::DoNotApply };

		/**
		 * @brief Parameter to store the lock-in amplifier's sensitivity/amplification setting.
		*/
		Param<double> Sensitivity{ *this, "Sensitivity", 1, 0 };

		/**
		 * @brief Parameter to store the phase in rad of the lock-in amplifier's demodulator.
		*/
		Param<double> Phase{ *this, "Phase", 0, 0, 2.0 * std::numbers::pi };

		/**
		 * @brief Parameter to store the time constant in seconds of the lock-in amplifier's
		 * low-poss filter.
		*/
		Param<double> TimeConstant{ *this, "TimeConstant", 1e-3, 0 };

		/**
		 * @brief Parameter to store the filter order/quality of the lock-in amplifier's
		 * low-poss filter.
		*/
		Param<unsigned int> FilterOrder{ *this, "FilterOrder", 1, 1, 10 };

		/**
		 * @brief Parameter to store the lock-in amplifier's trigger mode.
		*/
		Param<LockinAmplifierDefs::TriggerModeType> TriggerMode{ *this, "TriggerMode", LockinAmplifierDefs::TriggerModeType::ExternSingle };

		/**
		 * @brief Parameter to store the lock-in amplifier's trigger edge.
		*/
		Param<LockinAmplifierDefs::TriggerEdgeType> TriggerEdge{ *this, "TriggerEdge", LockinAmplifierDefs::TriggerEdgeType::Fall };

		/**
		 * @brief Parameter to store the lock-in amplifier's signal coordinate type to
		 * write to the data stream.
		*/
		Param<LockinAmplifierDefs::SignalType> Signal{ *this, "Signal", LockinAmplifierDefs::SignalType::R };

		/**
		 * @brief Parameter to store the lock-in amplifier's sampling rate in samples/s.
		*/
		Param<double> SamplingRate{ *this, "SamplingRate", 1000, 1 };

		/**
		 * @brief Parameter to store whether the lock-in amplifier's demodulator is enabled.
		*/
		Param<bool> Enable{ *this, "Enable", false };

	private:
		void ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>) override final { ConfigureParamsImpl(dispatch_tag<LockinAmplifierParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<LockinAmplifierParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>)
	};

	/**
	 * @brief Configurator class for @p LockinAmplifier
	*/
	class LockinAmplifierConfigurator : public DataStreamInstrumentConfigurator
	{
	public:
		using ObjectType = LockinAmplifier;
		using ParamsType = LockinAmplifierParams;

		LockinAmplifierConfigurator() = default;
		virtual ~LockinAmplifierConfigurator() = 0;
	};

	/**
	 * @brief Meta instrument for a lock-in amplifier based on the data stream
	 * meta instrument.
	*/
	class LockinAmplifier : public DataStreamInstrument
	{
	public:
		using ParamsType = LockinAmplifierParams;								//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = LockinAmplifierConfigurator;							//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = LockinAmplifierData;							//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = LockinAmplifierTasks::InitTask;					//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = LockinAmplifierTasks::ExitTask;					//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = LockinAmplifierTasks::UpdateTask;				//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Lock-in Amplifier"; }	//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }				//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		LockinAmplifier(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: DataStreamInstrument(OwnerThreadID, std::move(Params)) {}

		virtual ~LockinAmplifier() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(100); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		*/
		///@{
		/**
		 * @brief Determines the unit of the lock-in amplifier's sensitivity/amplification setting
		 * and returns a human-readable string to represent that unit.
		 * @return Unit string corresponding to the lock-in amplifier's sensitivity/amplification setting
		*/
		virtual const char* GetSensitivityUnitString() const noexcept = 0;
		///@}

		/**
		 * @brief Retrieves the current lock-in amplifier settings and stores them
		 * in the instrument parameters to save them to the project file. The
		 * function updates the parameters belonging to @p LockinAmplifierParams.
		 * Then, it calls @p PersistDataToParamsImpl() making use of the tag
		 * dispatch mechanism (refer to DynExp::ParamsBase::dispatch_tag) to let
		 * derived classes store additional settings they are responsible for in the
		 * instrument parameters.
		*/
		void PersistDataToParams() const;

		/**
		 * @brief Applies the lock-in amplifier settings stored in the instrument
		 * parameters to the physical device. The function only calls
		 * @p ApplyFromParamsImpl() making use of the tag dispatch mechanism
		 * (refer to DynExp::ParamsBase::dispatch_tag) to let derived classes
		 * apply the settings they are responsible for (by knowing how to apply them).
		*/
		void ApplyFromParams() const;

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{		
		/**
		 * @brief Sets the lock-in amplifier's sensitivity/amplification.
		 * @param Sensitivity Sensitivity/amplification in units as determined by
		 * @p GetSensitivityUnitString().
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetSensitivity(double Sensitivity, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Makes the lock-in amplifier automatically set its sensitivity/amplification.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void AutoAdjustSensitivity(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Sets the phase of the lock-in amplifier's demodulator.
		 * @param Phase Phase in rad
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetPhase(double Phase, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Makes the lock-in amplifier automatically set its demodulator's phase.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void AutoAdjustPhase(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Sets the time constant of the lock-in amplifier's low-poss filter.
		 * @param TimeConstant Time constant in seconds
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetTimeConstant(double TimeConstant, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Sets the filter order/quality of the lock-in amplifier's low-poss filter.
		 * @param FilterOrder Filter order/quality
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetFilterOrder(uint8_t FilterOrder, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Sets the lock-in amplifier's trigger mode.
		 * @param TriggerMode Trigger mode
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetTriggerMode(LockinAmplifierDefs::TriggerModeType TriggerMode, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Sets the lock-in amplifier's trigger edge.
		 * @param TriggerEdge Edge to trigger on
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetTriggerEdge(LockinAmplifierDefs::TriggerEdgeType TriggerEdge, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Sets the lock-in amplifier's signal coordinate type to read out.
		 * @param SignalType Signal coordinate to read out and to write to the instrument's data stream
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetSignalType(LockinAmplifierDefs::SignalType SignalType, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Sets the lock-in amplifier's sampling rate.
		 * @param SamplingRate Sampling rate in samples/s
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetSamplingRate(double SamplingRate, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Enables or disables the lock-in amplifier's demodulator.
		 * @param Enable Pass true to enable or false to disable the demodulator.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetEnable(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Forces (starts) the data acquisition ignoring the trigger.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void ForceTrigger(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;
		///@}

	private:
		void ResetImpl(dispatch_tag<DataStreamInstrument>) override final;
		virtual void ResetImpl(dispatch_tag<LockinAmplifier>) = 0;				//!< @copydoc ResetImpl(dispatch_tag<DataStreamInstrument>)

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Refer to @p PersistDataToParams(). Using tag dispatch mechanism to ensure that
		 * @p PersistDataToParamsImpl() of every derived class gets called - starting from
		 * @p LockinAmplifier, descending the inheritance hierarchy.
		*/
		virtual void PersistDataToParamsImpl(dispatch_tag<LockinAmplifier>) const {};

		/**
		 * @brief Refer to @p ApplyFromParams(). Using tag dispatch mechanism to ensure that
		 * @p ApplyFromParamsImpl() of every derived class gets called - starting from
		 * @p LockinAmplifier, descending the inheritance hierarchy.
		*/
		virtual void ApplyFromParamsImpl(dispatch_tag<LockinAmplifier>) const = 0;
		///@}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<LockinAmplifierTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<LockinAmplifierTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<LockinAmplifierTasks::UpdateTask>(); }
	};
}