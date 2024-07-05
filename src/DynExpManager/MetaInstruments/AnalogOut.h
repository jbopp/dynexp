// This file is part of DynExp.

/**
 * @file AnalogOut.h
 * @brief Defines a meta instrument for a single analog output port to write
 * a data stream consisting of analog values to.
*/

#pragma once

#include "stdafx.h"
#include "OutputPort.h"

namespace DynExpInstr
{
	class AnalogOut;

	/**
	 * @brief Tasks for @p AnalogOut
	*/
	namespace AnalogOutTasks
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

			/** @name Override
			 * Override by derived tasks.
			*/
			///@{
			/**
			 * @brief Indicates whether value limits are to be applied to the data stream
			 * of the @p AnalogOut instrument.
			 * @return Returnstrue if limits are to be applied, false otherwise.
			*/
			virtual bool ApplyLimits() const noexcept { return true; }
			///@}
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
	 * @brief Data class for @p AnalogOut
	*/
	class AnalogOutData : public OutputPortData
	{
	public:
		using SampleStreamType = AnalogSampleStream;					//!< Data stream type this data stream instrument should operate on.

		/**
		 * @copybrief DataStreamInstrumentData::DataStreamInstrumentData
		 * @param BufferSizeInSamples Initial buffer size of a data stream of type
		 * @p SampleStreamType to create for the related @p DataStreamInstrument
		 * instance to operate on
		*/
		AnalogOutData(size_t BufferSizeInSamples = 1) : OutputPortData(std::make_unique<SampleStreamType>(BufferSizeInSamples)) {}
		
		/**
		 * @copydoc DataStreamInstrumentData::DataStreamInstrumentData
		*/
		AnalogOutData(DataStreamBasePtrType&& SampleStream) : OutputPortData(std::move(SampleStream)) {}
		
		virtual ~AnalogOutData() = default;

	private:
		void ResetImpl(dispatch_tag<OutputPortData>) override final;
		virtual void ResetImpl(dispatch_tag<AnalogOutData>) {};			//!< @copydoc ResetImpl(dispatch_tag<OutputPortData>)
	};

	/**
	 * @brief Parameter class for @p AnalogOut
	*/
	class AnalogOutParams : public OutputPortParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p AnalogOut instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		AnalogOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: OutputPortParams(ID, Core) {}

		virtual ~AnalogOutParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "AnalogOutParams"; }

		/**
		 * @brief Default value to be written to the output port after initialization and
		 * after termination of this instrument.
		*/
		Param<ParamsConfigDialog::NumberType> DefaultValue = { *this, "DefaultValue", "Default value",
			"Value to be written to this output port after it has been initialized and before it is stopped.", false,
			0, std::numeric_limits<ParamsConfigDialog::NumberType>::lowest(), std::numeric_limits<ParamsConfigDialog::NumberType>::max(), 1, 3 };

		/**
		 * @brief In contrast to the hardware limits (minimal value the underlying hardware
		 * can generate), this value denotes a user-defined setting. Each output sample is
		 * validated against this value in order to prevent misconfigured modules from
		 * destroying connected physical hardware.
		 * Refer to OutputPort::GetUserMinValue().
		*/
		Param<ParamsConfigDialog::NumberType> MinValue = { *this, "MinValue", "Minimal value",
			"Lower limit the values written to this output port must not exceed.", true,
			-5, std::numeric_limits<ParamsConfigDialog::NumberType>::lowest(), std::numeric_limits<ParamsConfigDialog::NumberType>::max(), 1, 3 };

		/**
		 * @brief In contrast to the hardware limits (maximal value the underlying hardware
		 * can generate), this value denotes a user-defined setting. Each output sample is
		 * validated against this value in order to prevent misconfigured modules from
		 * destroying connected physical hardware.
		 * Refer to OutputPort::GetUserMaxValue().
		*/
		Param<ParamsConfigDialog::NumberType> MaxValue = { *this, "MaxValue", "Maximal value",
			"Upper limit the values written to this output port must not exceed.", true,
			5, std::numeric_limits<ParamsConfigDialog::NumberType>::lowest(), std::numeric_limits<ParamsConfigDialog::NumberType>::max(), 1, 3 };

	protected:
		void DisableUserEditable();												//!< @copydoc StreamSizeParamsExtension::DisableUserEditable

	private:
		void ConfigureParamsImpl(dispatch_tag<OutputPortParams>) override final { ConfigureParamsImpl(dispatch_tag<AnalogOutParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<AnalogOutParams>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<OutputPortParams>)
	};

	/**
	 * @brief Configurator class for @p AnalogOut
	*/
	class AnalogOutConfigurator : public OutputPortConfigurator
	{
	public:
		using ObjectType = AnalogOut;
		using ParamsType = AnalogOutParams;

		AnalogOutConfigurator() = default;
		virtual ~AnalogOutConfigurator() = 0;
	};

	/**
	 * @brief Meta instrument for a single analog output port based on the
	 * data stream, function generator, and generic output port meta instruments.
	*/
	class AnalogOut : public OutputPort
	{
	public:
		using ParamsType = AnalogOutParams;										//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = AnalogOutConfigurator;								//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = AnalogOutData;								//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = AnalogOutTasks::InitTask;							//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = AnalogOutTasks::ExitTask;							//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = AnalogOutTasks::UpdateTask;						//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Analog Out"; }			//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }				//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		AnalogOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: OutputPort(OwnerThreadID, std::move(Params)) {}
		
		virtual ~AnalogOut() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		 * For non-final methods, do not enforce noexcept to allow overriding functions which throw exceptions.
		*/
		///@{
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const override { return DataStreamInstrumentData::UnitType::Volt; }
		virtual DataStreamInstrumentData::ValueType GetUserMinValue() const override;
		virtual DataStreamInstrumentData::ValueType GetUserMaxValue() const override;

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
		virtual void Set(AnalogOutData::SampleStreamType::SampleType Sample, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Synchronized version of @p Set(), which blocks until @p Sample has been written.
		 * @param Sample Sample to write
		*/
		virtual void SetSync(AnalogOutData::SampleStreamType::SampleType Sample) const;

		/**
		 * @brief Writes AnalogOutParams::DefaultValue to the sample stream. @p CallbackFunc gets
		 * called after the task DataStreamInstrument::WriteData() has written the sample to the
		 * underlying hardware adapter.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetDefault(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;
		///@}

	private:
		virtual void OnPrepareExitChild() const override;

		void ResetImpl(dispatch_tag<OutputPort>) override final;
		virtual void ResetImpl(dispatch_tag<AnalogOut>) = 0;					//!< @copydoc ResetImpl(dispatch_tag<OutputPort>)

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<AnalogOutTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<AnalogOutTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<AnalogOutTasks::UpdateTask>(); }
	};
}