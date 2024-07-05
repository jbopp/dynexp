// This file is part of DynExp.

/**
 * @file InputPort.h
 * @brief Defines a generic input port meta instrument which is used to read
 * data from hardware and to write the data to a data stream.
*/

#pragma once

#include "stdafx.h"
#include "DataStreamInstrument.h"

namespace DynExpInstr
{
	class InputPort;

	/**
	 * @brief Tasks for @p InputPort
	*/
	namespace InputPortTasks
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
			 * @brief Determines whether to update the instrument's data stream size according
			 * to the instrument parameters.
			 * @return Return true if the size of the @p InputPort's data stream should
			 * be adjusted by the task automatically to the result of InputPort::GetStreamSizeParams(),
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
		 * @brief Task to reset the size of the related data stream instrument's stream
		 * to the result of InputPort::GetStreamSizeParams().
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
	 * @brief Data class for @p InputPort
	*/
	class InputPortData : public DataStreamInstrumentData
	{
	public:
		/**
		 * @copydoc DataStreamInstrumentData::DataStreamInstrumentData
		*/
		InputPortData(DataStreamBasePtrType&& SampleStream) : DataStreamInstrumentData(std::move(SampleStream)) {}
		
		virtual ~InputPortData() = default;

	private:
		void ResetImpl(dispatch_tag<DataStreamInstrumentData>) override final;
		virtual void ResetImpl(dispatch_tag<InputPortData>) {};		//!< @copydoc ResetImpl(dispatch_tag<DataStreamInstrumentData>)
	};

	/**
	 * @brief Parameter class for @p InputPort
	*/
	class InputPortParams : public DataStreamInstrumentParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p InputPort instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		InputPortParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: DataStreamInstrumentParams(ID, Core) {}

		virtual ~InputPortParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "InputPortParams"; }

	protected:
		void DisableUserEditable();												//!< @copydoc StreamSizeParamsExtension::DisableUserEditable

	private:
		void ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>) override final { ConfigureParamsImpl(dispatch_tag<InputPortParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<InputPortParams>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>)

		DummyParam Dummy = { *this };											//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p InputPort
	*/
	class InputPortConfigurator : public DataStreamInstrumentConfigurator
	{
	public:
		using ObjectType = InputPort;
		using ParamsType = InputPortParams;

		InputPortConfigurator() = default;
		virtual ~InputPortConfigurator() = 0;
	};

	/**
	 * @brief Generic input port meta instrument based on the data stream meta
	 * instrument to read data from hardware and to write the data to a data stream.
	*/
	class InputPort : public DataStreamInstrument
	{
	public:
		using ParamsType = InputPortParams;										//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = InputPortConfigurator;								//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = InputPortData;								//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = InputPortTasks::InitTask;							//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = InputPortTasks::ExitTask;							//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = InputPortTasks::UpdateTask;						//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Input Port"; }			//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }				//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		InputPort(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: DataStreamInstrument(OwnerThreadID, std::move(Params)) {}

		virtual ~InputPort() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		 * Do not enforce noexcept to allow overriding functions which throw exceptions.
		*/
		///@{
		/**
		 * @brief Retrieves the size of the instrument's data stream from the instrument parameters.
		 * Refer to InputPortTasks::InitTask::InitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::InitTask>, DynExp::InstrumentInstance&).
		 * @return Returns the user-defined buffer size to apply to the data stream. The default
		 * implementation returns a stream size of 1.
		*/
		virtual StreamSizeParamsExtension::ValueType GetStreamSizeParams() const { return { 1 }; }

		/**
		 * @brief Indicates the minimal value the underlying hardware can record.
		 * @return Returns the minimal value readable from the input port in units determined
		 * by DataStreamInstrument::GetValueUnit().
		*/
		virtual DataStreamInstrumentData::ValueType GetHardwareMinValue() const = 0;

		/**
		 * @brief Indicates the maximal value the underlying hardware can record.
		 * @return Returns the maximal value readable from the input port in units determined
		 * by DataStreamInstrument::GetValueUnit().
		*/
		virtual DataStreamInstrumentData::ValueType GetHardwareMaxValue() const = 0;

		/**
		 * @brief Indicates the value's resolution the underlying hardware can record.
		 * @return Returns the precision of values readable from the input port in units
		 * determined by DataStreamInstrument::GetValueUnit().
		*/
		virtual DataStreamInstrumentData::ValueType GetHardwareResolution() const = 0;
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		virtual void ResetStreamSize(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<InputPortTasks::ResetBufferSizeTask>(CallbackFunc); }
		///@}

	private:
		void ResetImpl(dispatch_tag<DataStreamInstrument>) override final;
		virtual void ResetImpl(dispatch_tag<InputPort>) = 0;					//!< @copydoc ResetImpl(dispatch_tag<DataStreamInstrument>)

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<InputPortTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<InputPortTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<InputPortTasks::UpdateTask>(); }
	};
}