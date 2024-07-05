// This file is part of DynExp.

/**
 * @file OutputPort.h
 * @brief Defines a generic output port meta instrument which is used to extract
 * data from a data stream and to write the data to hardware.
*/

#pragma once

#include "stdafx.h"
#include "FunctionGenerator.h"

namespace DynExpInstr
{
	class OutputPort;

	/**
	 * @brief Tasks for @p OutputPort
	*/
	namespace OutputPortTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		*/
		class InitTask : public FunctionGeneratorTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<FunctionGeneratorTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			
			/**
			 * @copydoc InitFuncImpl(dispatch_tag<FunctionGeneratorTasks::InitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}

			/** @name Override
			 * Override by derived tasks.
			*/
			///@{
			/**
			 * @brief Determines whether to update the instrument's data stream size according
			 * to the instrument parameters.
			 * @return Return true if the size of the @p OutputPort's data stream should
			 * be adjusted by the task automatically to the result of OutputPort::GetStreamSizeParams(),
			 * false otherwise.
			*/
			virtual bool ApplyDataStreamSizeFromParams() const noexcept { return true; }
			///@}
		};

		/**
		 * @copydoc DynExp::ExitTaskBase
		*/
		class ExitTask : public FunctionGeneratorTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<FunctionGeneratorTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final { ExitFuncImpl(dispatch_tag<ExitTask>(), Instance); }
			
			/**
			 * @copydoc ExitFuncImpl(dispatch_tag<FunctionGeneratorTasks::ExitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::UpdateTaskBase
		*/
		class UpdateTask : public FunctionGeneratorTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<FunctionGeneratorTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final { UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance); }
			
			/**
			 * @copydoc UpdateFuncImpl(dispatch_tag<FunctionGeneratorTasks::UpdateTask>, DynExp::InstrumentInstance&)
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @brief Task to reset the size of the related data stream instrument's stream
		 * to the result of OutputPort::GetStreamSizeParams().
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
	 * @brief Data class for @p OutputPort
	*/
	class OutputPortData : public FunctionGeneratorData
	{
	public:
		/**
		 * @copydoc DataStreamInstrumentData::DataStreamInstrumentData
		*/
		OutputPortData(DataStreamBasePtrType&& SampleStream) : FunctionGeneratorData(std::move(SampleStream)) {}
		
		virtual ~OutputPortData() = default;

	private:
		void ResetImpl(dispatch_tag<FunctionGeneratorData>) override final;
		virtual void ResetImpl(dispatch_tag<OutputPortData>) {};	//!< @copydoc ResetImpl(dispatch_tag<FunctionGeneratorData>)
	};

	/**
	 * @brief Parameter class for @p OutputPort
	*/
	class OutputPortParams : public FunctionGeneratorParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p OutputPort instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		OutputPortParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: FunctionGeneratorParams(ID, Core) {}
		
		virtual ~OutputPortParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "OutputPortParams"; }

	protected:
		void DisableUserEditable();												//!< @copydoc StreamSizeParamsExtension::DisableUserEditable

	private:
		void ConfigureParamsImpl(dispatch_tag<FunctionGeneratorParams>) override final { ConfigureParamsImpl(dispatch_tag<OutputPortParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<OutputPortParams>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<FunctionGeneratorParams>)

		DummyParam Dummy = { *this };											//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p OutputPort
	*/
	class OutputPortConfigurator : public FunctionGeneratorConfigurator
	{
	public:
		using ObjectType = OutputPort;
		using ParamsType = OutputPortParams;

		OutputPortConfigurator() = default;
		virtual ~OutputPortConfigurator() = 0;
	};

	/**
	 * @brief Generic output port meta instrument based on the data stream and
	 * function generator meta instruments to extract data from a data stream
	 * and to write the data to hardware.
	*/
	class OutputPort : public FunctionGenerator
	{
	public:
		using ParamsType = OutputPortParams;									//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = OutputPortConfigurator;								//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = OutputPortData;								//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = OutputPortTasks::InitTask;							//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = OutputPortTasks::ExitTask;							//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = OutputPortTasks::UpdateTask;						//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Output Port"; }			//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }				//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		OutputPort(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: FunctionGenerator(OwnerThreadID, std::move(Params)) {}

		virtual ~OutputPort() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		 * Do not enforce noexcept to allow overriding functions which throw exceptions.
		*/
		///@{
		/**
		 * @brief Retrieves the size of the instrument's data stream from the instrument parameters.
		 * Refer to OutputPortTasks::InitTask::InitFuncImpl(dispatch_tag<FunctionGeneratorTasks::InitTask>, DynExp::InstrumentInstance&).
		 * @return Returns the user-defined buffer size to apply to the data stream. The default
		 * implementation returns a stream size of 1.
		*/
		virtual StreamSizeParamsExtension::ValueType GetStreamSizeParams() const { return { 1 }; }

		/**
		 * @brief Indicates the minimal value the underlying hardware can generate.
		 * Also refer to @p GetUserMinValue().
		 * @return Returns the minimal value writeable to the output port in units determined
		 * by DataStreamInstrument::GetValueUnit().
		*/
		virtual DataStreamInstrumentData::ValueType GetHardwareMinValue() const = 0;

		/**
		 * @brief Indicates the maximal value the underlying hardware can generate.
		 * Also refer to @p GetUserMaxValue().
		 * @return Returns the maximal value writeable to the output port in units determined
		 * by DataStreamInstrument::GetValueUnit().
		*/
		virtual DataStreamInstrumentData::ValueType GetHardwareMaxValue() const = 0;

		/**
		 * @brief Indicates the value's resolution the underlying hardware can generate.
		 * @return Returns the precision of values writeable to the output port in units
		 * determined by DataStreamInstrument::GetValueUnit().
		*/
		virtual DataStreamInstrumentData::ValueType GetHardwareResolution() const = 0;

		/**
		 * @brief Indicates the minimal allowed value to generate as defined by the user/software.
		 * This value should be larger than the value returned by @p GetHardwareMinValue().
		 * @return Returns the minimal value writeable to the output port in units determined
		 * by DataStreamInstrument::GetValueUnit().
		*/
		virtual DataStreamInstrumentData::ValueType GetUserMinValue() const = 0;

		/**
		 * @brief Indicates the maximal allowed value to generate as defined by the user/software.
		 * This value should be smaller than the value returned by @p GetHardwareMaxValue().
		 * @return Returns the maximal value writeable to the output port in units determined
		 * by DataStreamInstrument::GetValueUnit().
		*/
		virtual DataStreamInstrumentData::ValueType GetUserMaxValue() const = 0;
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		virtual void ResetStreamSize(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<OutputPortTasks::ResetBufferSizeTask>(CallbackFunc); }
		///@}

	private:
		void ResetImpl(dispatch_tag<FunctionGenerator>) override final;
		virtual void ResetImpl(dispatch_tag<OutputPort>) = 0;					//!< @copydoc ResetImpl(dispatch_tag<FunctionGenerator>)

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<OutputPortTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<OutputPortTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<OutputPortTasks::UpdateTask>(); }
	};
}