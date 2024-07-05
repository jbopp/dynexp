// This file is part of DynExp.

/**
 * @file StreamManipulator.h
 * @brief Implementation of a module to process data stored in data stream instrument(s) with
 * a Python script and to write the resulting data back other data stream instrument(s).
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "PyModules.h"

namespace DynExpModule
{
	class StreamManipulator;

	/**
	 * @brief Type of a list of data stream instruments made available to Python
	*/
	using PyStreamListType = std::vector<DynExpInstr::PyDataStreamInstrument>;

	/**
	 * @brief Input data type passed to on_step() Python function
	*/
	struct PyStreamManipulatorInputData
	{
		/**
		 * @brief ID of the module invoking the on_step() Python function for stream manipulation
		*/
		DynExp::ItemIDType ModuleID{ DynExp::ItemIDNotSet };

		/**
		 * @brief Time point when the on_step() Python function was invoked last
		*/
		std::chrono::time_point<std::chrono::system_clock> LastExecutionTime{};

		/**
		 * @brief Vector of data stream instruments with input samples to be manipulated
		*/
		PyStreamListType InputStreams;

		/**
		 * @brief Vector of data stream instruments to write the resulting output samples to
		*/
		PyStreamListType OutputStreams;

		/**
		 * @brief Resets all member variables of this @p PyStreamManipulatorInputData instance
		 * back to their default values.
		*/
		void Reset();
	};

	/**
	 * @brief Output data type returned from on_step() Python function
	*/
	struct PyStreamManipulatorOutputData
	{
		/**
		 * @brief Time wo wait minimally before the on_step() Python function is called again
		 * when each input data stream has received new samples.
		*/
		std::chrono::milliseconds MinNextExecutionDelay{};

		/**
		 * @brief Time to wait maximally before the on_step() Python function is called again
		 * when the input data streams have not received new samples each.
		*/
		std::chrono::milliseconds MaxNextExecutionDelay{};

		/**
		 * @brief Maintaining the order of input data streams in
		 * PyStreamManipulatorInputData::InputStreams as passed to the on_step() Python
		 * function, this list contains for each input data stream the sample ID that has
		 * been consumed. These samples won't be passed to on_step() in its next call.
		*/
		std::vector<size_t> LastConsumedSampleIDsPerInputStream;

		/**
		 * @brief Resets all member variables of this @p PyStreamManipulatorOutputData instance
		 * back to their default values.
		*/
		void Reset();
	};

	/**
	 * @brief Data class for @p StreamManipulator
	*/
	class StreamManipulatorData : public DynExp::ModuleDataBase
	{
	public:
		StreamManipulatorData() { Init(); }
		virtual ~StreamManipulatorData() = default;

		auto& GetInputDataStreams() const noexcept { return InputDataStreams; }		//!< Getter for #InputDataStreams
		auto& GetInputDataStreams() noexcept { return InputDataStreams; }			//!< Getter for #InputDataStreams
		auto& GetOutputDataStreams() const noexcept { return OutputDataStreams; }	//!< Getter for #OutputDataStreams
		auto& GetOutputDataStreams() noexcept { return OutputDataStreams; }			//!< Getter for #OutputDataStreams

	private:
		/**
		 * @copydoc DynExp::ModuleDataBase::ResetImpl
		*/
		void ResetImpl(dispatch_tag<ModuleDataBase>) override final;

		/**
		 * @copydoc DynExp::ModuleDataBase::ResetImpl
		*/
		virtual void ResetImpl(dispatch_tag<StreamManipulatorData>) {};

		/**
		 * @brief Called by @p ResetImpl(dispatch_tag<DynExp::ModuleDataBase>) overridden by this
		 * class to initialize the data class instance. Currently, does not do anything.
		*/
		void Init();

		/**
		 * @brief Linked input data stream instruments whose samples are going to be used for calculations
		*/
		DynExp::LinkedObjectWrapperContainerList<DynExpInstr::DataStreamInstrument> InputDataStreams;

		/**
		 * @brief Linked output data stream instruments to write the computed new samples to
		*/
		DynExp::LinkedObjectWrapperContainerList<DynExpInstr::DataStreamInstrument> OutputDataStreams;
	};

	/**
	 * @brief Parameter class for @p StreamManipulator
	*/
	class StreamManipulatorParams : public DynExp::ModuleParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p StreamManipulator instance.
		*/
		StreamManipulatorParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : ModuleParamsBase(ID, Core) {}

		virtual ~StreamManipulatorParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "StreamManipulatorParams"; }

		/**
		 * @brief Parameter for data stream instruments containing input samples.
		 * Refer to StreamManipulatorData::InputDataStreams.
		*/
		ListParam<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>> InputDataStreams = { *this, GetCore().GetInstrumentManager(),
			"InputDataStreams", "Input data stream instrument(s)", "Data stream instruments to be used as data sources", DynExpUI::Icons::Instrument, true };
		
		/**
		 * @brief Parameter for data stream instruments containing output samples.
		 * Refer to StreamManipulatorData::OutputDataStreams.
		*/
		ListParam<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>> OutputDataStreams = { *this, GetCore().GetInstrumentManager(),
			"OutputDataStreams", "Output data stream instrument(s)", "Data stream instruments to be used as data sinks", DynExpUI::Icons::Instrument, true };
		
		/**
		 * @brief Path to the Python file containing the code, which performs the
		 * stream manipulation.
		 * @details The Python file can import Python modules at its beginning.
		 * Refer to the documentation main page for information on how to install Python
		 * modules.
		 * 
		 * Furthermore, the file can contain the functions @p on_init(input),
		 * @p on_step(input), and @p on_exit(input). @p input is the
		 * StreamManipulator::ManipulatorPyFuncInput instance and @p on_init() is expected
		 * to return an object assignable to StreamManipulator::ManipulatorPyFuncOutput.
		 * @p on_init() is called when the module is initialited, @p on_exit() is called
		 * when it is terminated, and @p on_step() is called periodically and as soon as new
		 * input samples are available according to PyStreamManipulatorOutputData::MinNextExecutionDelay
		 * and PyStreamManipulatorOutputData::MaxNextExecutionDelay.
		 * 
		 * To store local variables, they can be assigned to @p on_init() as attributes.
		 * Such attributes are accessible in @p on_step() and @p on_exit() as well.
		 * 
		 * Refer to the examples within this repository for different implementations
		 * of the Python functions described here.
		*/
		Param<ParamsConfigDialog::TextType> PythonCodePath = { *this, "PythonCodePath", "Python code path",
			"Path to a Python file containing the function which writes samples to the output streams based on the input streams' data", true, "", DynExp::TextUsageType::Code };

	private:
		/**
		 * @copydoc DynExp::ParamsBase::ConfigureParamsImpl
		*/
		void ConfigureParamsImpl(dispatch_tag<ModuleParamsBase>) override final {}
	};

	/**
	 * @brief Configurator class for @p StreamManipulator
	*/
	class StreamManipulatorConfigurator : public DynExp::ModuleConfiguratorBase
	{
	public:
		using ObjectType = StreamManipulator;
		using ParamsType = StreamManipulatorParams;

		StreamManipulatorConfigurator() = default;
		virtual ~StreamManipulatorConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<StreamManipulatorConfigurator>(ID, Core); }
	};

	/**
	 * @brief Module to process data stored in data stream instrument(s) with
	 * a Python script and to write the resulting data back other data stream instrument(s).
	*/
	class StreamManipulator : public DynExp::ModuleBase
	{
		/**
		 * @brief Sample stream type expected for input (StreamManipulatorData::InputDataStreams)
		 * and output (StreamManipulatorData::OutputDataStreams) data stream instruments.
		*/
		using SampleStreamType = DynExpInstr::CircularDataStreamBase;

		/**
		 * @brief Type of a pointer to a Python function bound by pybind11
		*/
		using PyFuncType = Util::PyObject<py::object>;

	public:
		using ParamsType = StreamManipulatorParams;								//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = StreamManipulatorConfigurator;						//!< @copydoc DynExp::Object::ConfigType
		using ModuleDataType = StreamManipulatorData;							//!< @copydoc DynExp::ModuleBase::ModuleDataType

		constexpr static auto Name() noexcept { return "Stream Manipulator"; }	//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }				//!< @copydoc DynExp::ModuleBase::Category

		/**
		 * @copydoc DynExp::ModuleBase::ModuleBase
		*/
		StreamManipulator(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: ModuleBase(OwnerThreadID, std::move(Params)) {}

		virtual ~StreamManipulator() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		/**
		 * @copydoc DynExp::ModuleBase::TreatModuleExceptionsAsWarnings
		 * @brief If an error occurs during initialization, terminate the module.
		*/
		bool TreatModuleExceptionsAsWarnings() const override { return IsInitialized; }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(1); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		/**
		 * @copydoc DynExp::Object::ResetImpl
		*/
		void ResetImpl(dispatch_tag<ModuleBase>) override final;

		/**
		 * @brief Performs a single manipulation step by preparing input data for a call to the
		 * Python function @p on_step(), by calling it, and by processing the data it returns.
		 * @param ModuleData Locked module data instance
		*/
		void Step(Util::SynchronizedPointer<ModuleDataType>& ModuleData);

		/** @name Events
		 * Event functions running in the module thread.
		*/
		///@{
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		///@}

		/**
		 * @brief Indicates whether the module has been fully initialized by finishing @p OnInit().
		 * The variable is atomic since it is revealed to other threads through
		 * @p TreatModuleExceptionsAsWarnings().
		*/
		mutable std::atomic<bool> IsInitialized = false;

		/**
		 * @brief Unique name of the Python function all the code of this @p StreamManipulator
		 * instance is declared in. The name contains the module's ID to ensure that multiple
		 * @p StreamManipulator instances can declare their own @p on_init(), @p on_step(), and
		 * @p on_exit() Python functions at the same time.
		*/
		std::string ManipulatorPyFuncName;

		mutable PyFuncType ManipulatorPyFuncInit;	//!< Handle to a Python function called on module initialization.
		mutable PyFuncType ManipulatorPyFuncStep;	//!< Handle to a Python function called for each manipulation step.
		mutable PyFuncType ManipulatorPyFuncExit;	//!< Handle to a Python function called on module termination.

		/**
		 * @brief Input data passed to the on_step() Python function
		*/
		mutable PyStreamManipulatorInputData ManipulatorPyFuncInput;

		/**
		 * @brief Output data returned from the on_step() Python function
		*/
		mutable PyStreamManipulatorOutputData ManipulatorPyFuncOutput;

		/**
		 * @brief Counts how often StreamManipulator::ModuleMainLoop() contiguously failed due
		 * to an exception of type Util::TimeoutException.
		*/
		size_t NumFailedUpdateAttempts = 0;

		/**
		 * @copydoc PyStreamManipulatorInputData::LastExecutionTime
		*/
		std::chrono::time_point<std::chrono::system_clock> LastManipulatorPyFuncExecution{};
	};
}