// This file is part of DynExp.

/**
 * @file DigitalIn.h
 * @brief Defines a meta instrument for a single digital input port to read
 * a data stream consisting of digital values from.
*/

#pragma once

#include "stdafx.h"
#include "InputPort.h"

namespace DynExpInstr
{
	class DigitalIn;

	/**
	 * @brief Tasks for @p DigitalIn
	*/
	namespace DigitalInTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		*/
		class InitTask : public InputPortTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<InputPortTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final { InitFuncImpl(dispatch_tag<InitTask>(), Instance); }
			
			/**
			 * @copydoc InitFuncImpl(dispatch_tag<InputPortTasks::InitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::ExitTaskBase
		*/
		class ExitTask : public InputPortTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<InputPortTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final { ExitFuncImpl(dispatch_tag<ExitTask>(), Instance); }
			
			/**
			 * @copydoc ExitFuncImpl(dispatch_tag<InputPortTasks::ExitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::UpdateTaskBase
		*/
		class UpdateTask : public InputPortTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<InputPortTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final { UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance); }
			
			/**
			 * @copydoc UpdateFuncImpl(dispatch_tag<InputPortTasks::UpdateTask>, DynExp::InstrumentInstance&)
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};
	}

	/**
	 * @brief Data class for @p DigitalIn
	*/
	class DigitalInData : public InputPortData
	{
	public:
		using SampleStreamType = DigitalSampleStream;					//!< Data stream type this data stream instrument should operate on.

		/**
		 * @copybrief DataStreamInstrumentData::DataStreamInstrumentData
		 * @param BufferSizeInSamples Initial buffer size of a data stream of type
		 * @p SampleStreamType to create for the related @p DataStreamInstrument
		 * instance to operate on
		*/
		DigitalInData(size_t BufferSizeInSamples = 1) : InputPortData(std::make_unique<SampleStreamType>(BufferSizeInSamples)) {}
		
		/**
		 * @copydoc DataStreamInstrumentData::DataStreamInstrumentData
		*/
		DigitalInData(DataStreamBasePtrType&& SampleStream) : InputPortData(std::move(SampleStream)) {}
		
		virtual ~DigitalInData() = default;

	private:
		void ResetImpl(dispatch_tag<InputPortData>) override final;
		virtual void ResetImpl(dispatch_tag<DigitalInData>) {};			//!< @copydoc ResetImpl(dispatch_tag<InputPortData>)
	};

	/**
	 * @brief Parameter class for @p DigitalIn
	*/
	class DigitalInParams : public InputPortParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p DigitalIn instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		DigitalInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: InputPortParams(ID, Core) {}

		virtual ~DigitalInParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "DigitalInParams"; }

	protected:
		void DisableUserEditable();												//!< @copydoc StreamSizeParamsExtension::DisableUserEditable

	private:
		void ConfigureParamsImpl(dispatch_tag<InputPortParams>) override final { ConfigureParamsImpl(dispatch_tag<DigitalInParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<DigitalInParams>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<InputPortParams>)

		DummyParam Dummy = { *this };											//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p DigitalIn
	*/
	class DigitalInConfigurator : public InputPortConfigurator
	{
	public:
		using ObjectType = DigitalIn;
		using ParamsType = DigitalInParams;

		DigitalInConfigurator() = default;
		virtual ~DigitalInConfigurator() = 0;
	};

	/**
	 * @brief Meta instrument for a single digital input port based on the
	 * data stream and generic input port meta instruments.
	*/
	class DigitalIn : public InputPort
	{
	public:
		using ParamsType = DigitalInParams;										//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = DigitalInConfigurator;								//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = DigitalInData;								//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = DigitalInTasks::InitTask;							//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = DigitalInTasks::ExitTask;							//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = DigitalInTasks::UpdateTask;						//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Digital In"; }			//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }				//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		DigitalIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: InputPort(OwnerThreadID, std::move(Params)) {}
		
		virtual ~DigitalIn() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		 * For non-final methods, do not enforce noexcept to allow overriding functions which throw exceptions.
		*/
		///@{
		virtual DataStreamInstrumentData::ValueType GetHardwareMinValue() const noexcept override final { return 0; }
		virtual DataStreamInstrumentData::ValueType GetHardwareMaxValue() const noexcept override final { return 1; }
		virtual DataStreamInstrumentData::ValueType GetHardwareResolution() const noexcept override final { return 1; }
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const noexcept override final { return DataStreamInstrumentData::UnitType::LogicLevel; }
		
		/**
		 * @brief Retrieves sample stream settings of the instrument's data stream from the instrument's
		 * or from the underlying hardware's parameters.
		 * @return Returns user-defined sample stream settings. The default implementation returns a
		 * default-constructed NumericSampleStreamParamsExtension::ValueType instance.
		*/
		virtual NumericSampleStreamParamsExtension::ValueType GetNumericSampleStreamParams() const { return {}; }
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		/**
		 * @brief Reads one sample from the sample stream. @p CallbackFunc gets called after the task
		 * DataStreamInstrument::ReadData() has read a sample from the underlying hardware adapter.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @return Sample read from the data stream
		*/
		virtual DigitalInData::SampleStreamType::SampleType Get(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Synchronized version of @p Get(), which blocks until a DataStreamInstrument::ReadData()
		 * task getting enqueued has finished. The sample is read after this task has finished.
		 * @return Sample read from the data stream
		*/
		virtual DigitalInData::SampleStreamType::SampleType GetSync() const;
		///@}

	private:
		void ResetImpl(dispatch_tag<InputPort>) override final;
		virtual void ResetImpl(dispatch_tag<DigitalIn>) = 0;					//!< @copydoc ResetImpl(dispatch_tag<InputPort>)

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<DigitalInTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<DigitalInTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<DigitalInTasks::UpdateTask>(); }
	};
}