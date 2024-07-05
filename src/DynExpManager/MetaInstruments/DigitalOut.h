// This file is part of DynExp.

/**
 * @file DigitalOut.h
 * @brief Defines a meta instrument for a single digital output port to write
 * a data stream consisting of digital values to.
*/

#pragma once

#include "stdafx.h"
#include "OutputPort.h"

namespace DynExpInstr
{
	class DigitalOut;

	/**
	 * @brief Tasks for @p DigitalOut
	*/
	namespace DigitalOutTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		*/
		class InitTask : public OutputPortTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<OutputPortTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			
			/**
			 * @copydoc InitFuncImpl(dispatch_tag<OutputPortTasks::InitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::ExitTaskBase
		*/
		class ExitTask : public OutputPortTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<OutputPortTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;
			
			/**
			 * @copydoc ExitFuncImpl(dispatch_tag<OutputPortTasks::ExitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::UpdateTaskBase
		*/
		class UpdateTask : public OutputPortTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<OutputPortTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final { UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance); }
			
			/**
			 * @copydoc UpdateFuncImpl(dispatch_tag<OutputPortTasks::UpdateTask>, DynExp::InstrumentInstance&)
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};
	}

	/**
	 * @brief Data class for @p DigitalOut
	*/
	class DigitalOutData : public OutputPortData
	{
	public:
		using SampleStreamType = DigitalSampleStream;					//!< Data stream type this data stream instrument should operate on.

		/**
		 * @copybrief DataStreamInstrumentData::DataStreamInstrumentData
		 * @param BufferSizeInSamples Initial buffer size of a data stream of type
		 * @p SampleStreamType to create for the related @p DataStreamInstrument
		 * instance to operate on
		*/
		DigitalOutData(size_t BufferSizeInSamples = 1) : OutputPortData(std::make_unique<SampleStreamType>(BufferSizeInSamples)) {}
		
		/**
		 * @copydoc DataStreamInstrumentData::DataStreamInstrumentData
		*/
		DigitalOutData(DataStreamBasePtrType&& SampleStream) : OutputPortData(std::move(SampleStream)) {}
		
		virtual ~DigitalOutData() = default;

	private:
		void ResetImpl(dispatch_tag<OutputPortData>) override final;
		virtual void ResetImpl(dispatch_tag<DigitalOutData>) {};		//!< @copydoc ResetImpl(dispatch_tag<OutputPortData>)
	};

	/**
	 * @brief Parameter class for @p DigitalOut
	*/
	class DigitalOutParams : public OutputPortParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p DigitalOut instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		DigitalOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: OutputPortParams(ID, Core) {}

		virtual ~DigitalOutParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "DigitalOutParams"; }

		/**
		 * @brief Default value to be written to the output port after initialization and
		 * after termination of this instrument.
		*/
		Param<ParamsConfigDialog::NumberType> DefaultValue = { *this, "DefaultValue",  "Default value",
			"Value to be written to this output port after it has been initialized and before it is stopped.", false, 0, 0, 1, 1, 0 };

	protected:
		void DisableUserEditable();												//!< @copydoc StreamSizeParamsExtension::DisableUserEditable

	private:
		void ConfigureParamsImpl(dispatch_tag<OutputPortParams>) override final { ConfigureParamsImpl(dispatch_tag<DigitalOutParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<DigitalOutParams>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<OutputPortParams>)
	};

	/**
	 * @brief Configurator class for @p DigitalOut
	*/
	class DigitalOutConfigurator : public OutputPortConfigurator
	{
	public:
		using ObjectType = DigitalOut;
		using ParamsType = DigitalOutParams;

		DigitalOutConfigurator() = default;
		virtual ~DigitalOutConfigurator() = 0;
	};

	/**
	 * @brief Meta instrument for a single digital output port based on the
	 * data stream, function generator, and generic output port meta instruments.
	*/
	class DigitalOut : public OutputPort
	{
	public:
		using ParamsType = DigitalOutParams;									//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = DigitalOutConfigurator;								//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = DigitalOutData;								//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = DigitalOutTasks::InitTask;							//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = DigitalOutTasks::ExitTask;							//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = DigitalOutTasks::UpdateTask;						//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Digital Out"; }			//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }				//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		DigitalOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: OutputPort(OwnerThreadID, std::move(Params)) {}
		
		virtual ~DigitalOut() = 0;

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
		virtual DataStreamInstrumentData::ValueType GetUserMinValue() const override { return GetHardwareMinValue(); }
		virtual DataStreamInstrumentData::ValueType GetUserMaxValue() const override { return GetHardwareMaxValue(); }
		
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
		 * @brief Writes one sample to the sample stream. @p CallbackFunc gets called after the task
		 * DataStreamInstrument::WriteData() has written the sample to the underlying hardware adapter.
		 * @param Sample Sample to write
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Set(DigitalOutData::SampleStreamType::SampleType Sample, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Synchronized version of @p Set(), which blocks until @p Sample has been written.
		 * @param Sample Sample to write
		*/
		virtual void SetSync(DigitalOutData::SampleStreamType::SampleType Sample) const;

		/**
		 * @brief Writes DigitalOutParams::DefaultValue to the sample stream. @p CallbackFunc gets
		 * called after the task DataStreamInstrument::WriteData() has written the sample to the
		 * underlying hardware adapter.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetDefault(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;
		///@}

	private:
		virtual void OnPrepareExitChild() const override;

		void ResetImpl(dispatch_tag<OutputPort>) override final;
		virtual void ResetImpl(dispatch_tag<DigitalOut>) = 0;					//!< @copydoc ResetImpl(dispatch_tag<OutputPort>)

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<DigitalOutTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<DigitalOutTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<DigitalOutTasks::UpdateTask>(); }
	};
}