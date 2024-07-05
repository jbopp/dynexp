// This file is part of DynExp.

/**
 * @file TimeTagger.h
 * @brief Defines a time tagging meta instrument to assign timestamps to
 * (e.g. photon) detection events.
*/

#pragma once

#include "stdafx.h"
#include "DataStreamInstrument.h"

namespace DynExpInstr
{
	class TimeTagger;

	/**
	 * @brief Tasks for @p TimeTagger
	*/
	namespace TimeTaggerTasks
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

			/** @name Override
			 * Override by derived tasks.
			*/
			///@{
			/**
			 * @brief Determines whether to update the @p TimeTagger's data stream size according
			 * to the instrument parameters.
			 * @return Return true if the size of the @p TimeTagger's data stream should
			 * be adjusted by the task automatically to TimeTaggerParams::StreamSizeParams::StreamSize,
			 * false otherwise.
			*/
			virtual bool ApplyDataStreamSizeFromParams() const noexcept { return true; }
			///@}
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

		/**
		 * @brief Task to reset the size of the @p TimeTagger's data stream
		 * to TimeTaggerParams::StreamSizeParams::StreamSize.
		*/
		class ResetBufferSizeTask final : public DynExp::TaskBase
		{
		public:
			/**
			 * @brief Constructs a @p ResetBufferSizeTask instance.
			 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
			*/
			ResetBufferSizeTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	/**
	 * @brief Data class for @p TimeTagger
	*/
	class TimeTaggerData : public DataStreamInstrumentData
	{
	public:
		/**
		 * @brief Type to store results of a g^(2) measurement
		*/
		struct HBTResultsType
		{
			/**
			 * @brief List type to store the samples g^(2)(t)
			*/
			using ResultVectorType = std::vector<BasicSample>;

			/**
			 * @brief Constructs a @p HBTResultsType instance calling @p Reset().
			*/
			HBTResultsType() { Reset(); }

			/**
			 * @brief Sets #Enabled to false, #EventCounts as well as
			 * #IntegrationTime to zero, and clears #ResultVector.
			*/
			void Reset();

			bool Enabled;								//!< Indicates whether a g^(2) measurement instead of timestamp readout is active.
			long long EventCounts;						//!< Indicates the amount of time-tagged events used to calculate g^(2).
			std::chrono::microseconds IntegrationTime;	//!< Indicates the duration for which the g^(2) measurement is running.

			ResultVectorType ResultVector;				//!< Stores the g^(2)(t) samples.
		};

		/**
		 * @brief Type to determine whether the instrument reads out all time-tagged
		 * events or summarized count rates.
		 * Not a strongly-typed enum to allow using the enumeration in a
		 * DynExp::ParamsBase::Param in class DynExpInstr::TimeTaggerParams.
		*/
		enum StreamModeType {
			Counts,		//!< Read out summarized count rates in Hz.
			Events		//!< Read out the timestamps of all time-tagged events.
		};

		using SampleStreamType = BasicSampleStream;		//!< Data stream type this data stream instrument operates on.

		/**
		 * @brief Constructs a @p TimeTaggerData instance.
		 * @param BufferSizeInSamples Initial buffer size of a data stream of type
		 * @p SampleStreamType to create for the related @p DataStreamInstrument
		 * instance to operate on
		*/
		TimeTaggerData(size_t BufferSizeInSamples = 1)
			: DataStreamInstrumentData(std::make_unique<SampleStreamType>(BufferSizeInSamples)),
			StreamMode(StreamModeType::Counts), StreamModeChanged(true) {}

		/**
		 * @brief Constructs a @p TimeTaggerData instance.
		 * @copydetails DataStreamInstrumentData::DataStreamInstrumentData
		*/
		TimeTaggerData(DataStreamBasePtrType&& SampleStream)
			: DataStreamInstrumentData(std::move(SampleStream)),
			StreamMode(StreamModeType::Counts), StreamModeChanged(true) {}

		virtual ~TimeTaggerData() = default;

		auto GetStreamMode() const noexcept { return StreamMode; }					//!< Getter for #StreamMode
		void SetStreamMode(StreamModeType StreamMode) const noexcept;				//!< Setter for #StreamMode. Adjustable by modules. Sets #StreamModeChanged to true.
		bool GetStreamModeChanged() const noexcept { return StreamModeChanged; }	//!< Getter for #StreamModeChanged
		void ResetStreamMode() const noexcept { StreamModeChanged = true; }			//!< Sets #StreamModeChanged to true. Callable by modules.
		void ClearStreamModeChanged() noexcept { StreamModeChanged = false; }		//!< Sets #StreamModeChanged to false.

		auto& GetHBTResults() const noexcept { return HBTResults; }					//!< Getter for #HBTResults
		auto& GetHBTResults() noexcept { return HBTResults; }						//!< Getter for #HBTResults

	private:
		void ResetImpl(dispatch_tag<DataStreamInstrumentData>) override final;
		virtual void ResetImpl(dispatch_tag<TimeTaggerData>) {};					//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>)
	
		/**
		 * @brief Stream mode of the @p TimeTagger. Derived instruments are supposed to
		 * apply the stream mode to the phyiscal device in their update task derived from
		 * TimeTaggerTasks::UpdateTask if #StreamModeChanged is true. After that, they
		 * should call @p ClearStreamModeChanged(). Also refer to #StreamModeChanged.
		 * Logical const-ness: allow modules to communicate to this instrument by
		 * adjusting #StreamMode.
		*/
		mutable StreamModeType StreamMode;

		/**
		 * @brief Indicates whether #StreamMode has to be applied to the phyiscal device.
		 * Also refer to #StreamMode.
		 * Logical const-ness: allow modules to communicate to this instrument by
		 * adjusting #StreamMode.
		*/
		mutable bool StreamModeChanged;

		/**
		 * @brief Stores the results of a g^(2) measurement.
		*/
		HBTResultsType HBTResults;
	};

	/**
	 * @brief Parameter class for @p TimeTagger
	*/
	class TimeTaggerParams : public DataStreamInstrumentParams
	{
	public:
		/**
		 * @brief Maps description strings to the TimeTaggerData::StreamModeType enum's items.
		 * @return List containing the description-value mapping
		*/
		static Util::TextValueListType<TimeTaggerData::StreamModeType> StreamModeTypeStrList();

		/**
		 * @brief Constructs the parameters for a @p TimeTagger instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		TimeTaggerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: DataStreamInstrumentParams(ID, Core), StreamSizeParams(*this) {}

		virtual ~TimeTaggerParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "TimeTaggerParams"; }
		
		/**
		 * @brief Parameter to store the size of the time tagger's sample stream in samples.
		*/
		StreamSizeParamsExtension StreamSizeParams;

		/**
		 * @brief Parameter to store the time tagger's stream mode.
		*/
		Param<TimeTaggerData::StreamModeType> StreamMode = { *this, StreamModeTypeStrList(), "StreamMode", "Stream mode",
			"Determines what to store in the sample stream of this instrument.", true, TimeTaggerData::StreamModeType::Counts };

	private:
		void ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>) override final { ConfigureParamsImpl(dispatch_tag<TimeTaggerParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<TimeTaggerParams>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>)
	};

	/**
	 * @brief Configurator class for @p TimeTagger
	*/
	class TimeTaggerConfigurator : public DataStreamInstrumentConfigurator
	{
	public:
		using ObjectType = TimeTagger;
		using ParamsType = TimeTaggerParams;

		TimeTaggerConfigurator() = default;
		virtual ~TimeTaggerConfigurator() = 0;
	};

	/**
	 * @brief Meta instrument for a time tagging device to assign timestamps to
	 * (e.g. photon) detection events based on the data stream meta instrument.
	*/
	class TimeTagger : public DataStreamInstrument
	{
	public:
		using ParamsType = TimeTaggerParams;								//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = TimeTaggerConfigurator;							//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = TimeTaggerData;							//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = TimeTaggerTasks::InitTask;						//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = TimeTaggerTasks::ExitTask;						//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = TimeTaggerTasks::UpdateTask;					//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Time Tagger"; }		//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }			//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		TimeTagger(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: DataStreamInstrument(OwnerThreadID, std::move(Params)) {}

		virtual ~TimeTagger() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(10); }
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const noexcept override { return DataStreamInstrumentData::UnitType::Counts; }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		*/
		///@{
		/**
		 * @brief Determines the minimal configurable threshold voltage, which needs
		 * to be exceeded at the time tagger's physical input to regard a signal
		 * change as an event.
		 * @return Minimal configurable threshold voltage in volts
		*/
		virtual double GetMinThresholdInVolts() const noexcept = 0;

		/**
		 * @brief Determines the maximal configurable threshold voltage, which needs
		 * to be exceeded at the time tagger's physical input to regard a signal
		 * change as an event.
		 * @return Maximal configurable threshold voltage in V
		*/
		virtual double GetMaxThresholdInVolts() const noexcept = 0;

		/**
		 * @brief Determines the time tagger's time resolution in assigning
		 * timestamps to events.
		 * @return Timing resolution in ps
		*/
		virtual Util::picoseconds GetResolution() const = 0;

		/**
		 * @brief Determines the time tagger's internal buffer size for storing
		 * time-tagged events.
		 * @return Buffer size in samples
		*/
		virtual size_t GetBufferSize() const = 0;
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		virtual void ResetStreamSize(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<TimeTaggerTasks::ResetBufferSizeTask>(CallbackFunc); }

		/**
		 * @brief Clears the recorded time-tagged events from the hardware buffer
		 * of the underlying hardware adapter.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Clear(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Configures the input channel of the physical device related to this instrument.
		 * @param UseRisingEdge Pass true to regard rising edges as events, false to regard
		 * falling edges as events.
		 * @param ThresholdInVolts Threshold voltage, which needs to be exceeded at the time
		 * tagger's physical input to regard a signal change as an event.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void ConfigureInput(bool UseRisingEdge, double ThresholdInVolts, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Configures the time tagger's exposure time. This is e.g. relevant to determine
		 * count rates in the TimeTaggerData::Counts stream mode.
		 * @param ExposureTime Exposure/integration time in ps
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetExposureTime(Util::picoseconds ExposureTime, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Configures the time tagger's coincidence window.
		 * @param CoincidenceWindow Coincidence window in ps
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetCoincidenceWindow(Util::picoseconds CoincidenceWindow, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Configures the time tagger's channel delay. This is useful to compensate
		 * different signal delays between the detector and the physical time tagger device.
		 * @param Delay Time delay in ps
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetDelay(Util::picoseconds Delay, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Enables or disables the time tagger's HBT unit for g^(2) measurements.
		 * @param Enable Pass true to enable and false to disable g^(2) measurements.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetHBTActive(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Configures the time tagger's HBT unit for g^(2) measurements.
		 * @param BinWidth Width of time bins in ps of the g^(2)(t) function
		 * @param BinCount Number of time bins in the g^(2)(t) function
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void ConfigureHBT(Util::picoseconds BinWidth, size_t BinCount, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Resets the time tagger's HBT unit for g^(2) measurements.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void ResetHBT(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;
		///@}

	private:
		void ResetImpl(dispatch_tag<DataStreamInstrument>) override final;
		virtual void ResetImpl(dispatch_tag<TimeTagger>) = 0;				//!< @copydoc ResetImpl(dispatch_tag<DataStreamInstrument>)

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<TimeTaggerTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<TimeTaggerTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<TimeTaggerTasks::UpdateTask>(); }
	};
}