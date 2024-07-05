// This file is part of DynExp.

/**
 * @file AnalogIn.h
 * @brief Defines a meta instrument for a single analog input port to read
 * a data stream consisting of analog values from.
*/

#pragma once

#include "stdafx.h"
#include "InputPort.h"

namespace DynExpInstr
{
	class AnalogIn;

	/**
	 * @brief Tasks for @p AnalogIn
	*/
	namespace AnalogInTasks
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
	 * @brief Data class for @p AnalogIn
	*/
	class AnalogInData : public InputPortData
	{
	public:
		using SampleStreamType = AnalogSampleStream;					//!< Data stream type this data stream instrument should operate on.

		/**
		 * @copybrief DataStreamInstrumentData::DataStreamInstrumentData
		 * @param BufferSizeInSamples Initial buffer size of a data stream of type
		 * @p SampleStreamType to create for the related @p DataStreamInstrument
		 * instance to operate on
		*/
		AnalogInData(size_t BufferSizeInSamples = 1) : InputPortData(std::make_unique<SampleStreamType>(BufferSizeInSamples)) {}
		
		/**
		 * @copydoc DataStreamInstrumentData::DataStreamInstrumentData
		*/
		AnalogInData(DataStreamBasePtrType&& SampleStream) : InputPortData(std::move(SampleStream)) {}
		
		virtual ~AnalogInData() = default;

	private:
		void ResetImpl(dispatch_tag<InputPortData>) override final;
		virtual void ResetImpl(dispatch_tag<AnalogInData>) {};			//!< @copydoc ResetImpl(dispatch_tag<InputPortData>)
	};

	/**
	 * @brief Parameter class for @p AnalogIn
	*/
	class AnalogInParams : public InputPortParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p AnalogIn instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		AnalogInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: InputPortParams(ID, Core) {}

		virtual ~AnalogInParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "AnalogInParams"; }

	protected:
		void DisableUserEditable();												//!< @copydoc StreamSizeParamsExtension::DisableUserEditable

	private:
		void ConfigureParamsImpl(dispatch_tag<InputPortParams>) override final { ConfigureParamsImpl(dispatch_tag<AnalogInParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<AnalogInParams>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<InputPortParams>)

		DummyParam Dummy = { *this };											//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p AnalogIn
	*/
	class AnalogInConfigurator : public InputPortConfigurator
	{
	public:
		using ObjectType = AnalogIn;
		using ParamsType = AnalogInParams;

		AnalogInConfigurator() = default;
		virtual ~AnalogInConfigurator() = 0;
	};

	/**
	 * @brief Meta instrument for a single analog input port based on the
	 * data stream and generic input port meta instruments.
	*/
	class AnalogIn : public InputPort
	{
	public:
		using ParamsType = AnalogInParams;										//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = AnalogInConfigurator;								//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = AnalogInData;								//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = AnalogInTasks::InitTask;							//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = AnalogInTasks::ExitTask;							//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = AnalogInTasks::UpdateTask;						//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Analog In"; }			//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }				//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		AnalogIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: InputPort(OwnerThreadID, std::move(Params)) {}
		
		virtual ~AnalogIn() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		 * For non-final methods, do not enforce noexcept to allow overriding functions which throw exceptions.
		*/
		///@{
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const override { return DataStreamInstrumentData::UnitType::Volt; }

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
		virtual AnalogInData::SampleStreamType::SampleType Get(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Synchronized version of @p Get(), which blocks until a DataStreamInstrument::ReadData()
		 * task getting enqueued has finished. The sample is read after this task has finished.
		 * @return Sample read from the data stream
		*/
		virtual AnalogInData::SampleStreamType::SampleType GetSync() const;
		///@}

	private:
		void ResetImpl(dispatch_tag<InputPort>) override final;
		virtual void ResetImpl(dispatch_tag<AnalogIn>) = 0;						//!< @copydoc ResetImpl(dispatch_tag<InputPort>)

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<AnalogInTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<AnalogInTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<AnalogInTasks::UpdateTask>(); }
	};
}