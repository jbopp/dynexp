// This file is part of DynExp.

#include "stdafx.h"
#include "StreamManipulator.h"

PYBIND11_MAKE_OPAQUE(DynExpModule::PyStreamListType);
PYBIND11_MAKE_OPAQUE(decltype(DynExpModule::PyStreamManipulatorOutputData::LastConsumedSampleIDsPerInputStream));

PYBIND11_EMBEDDED_MODULE(PyModuleStreamManipulator, m)
{
	using namespace DynExpModule;

	py::bind_vector<DynExpModule::PyStreamListType>(m, "StreamListType");
	py::bind_vector<decltype(DynExpModule::PyStreamManipulatorOutputData::LastConsumedSampleIDsPerInputStream)>(m, "SampleIDListType");

	py::class_<PyStreamManipulatorInputData>(m, "InputData")
		.def(py::init<>())
		.def_readonly("ModuleID", &PyStreamManipulatorInputData::ModuleID)
		.def_readonly("LastExecutionTime", &PyStreamManipulatorInputData::LastExecutionTime)
		.def_readwrite("InputStreams", &PyStreamManipulatorInputData::InputStreams)
		.def_readwrite("OutputStreams", &PyStreamManipulatorInputData::OutputStreams);

	py::class_<PyStreamManipulatorOutputData>(m, "OutputData")
		.def(py::init<>())
		.def_readwrite("MinNextExecutionDelay", &PyStreamManipulatorOutputData::MinNextExecutionDelay)
		.def_readwrite("MaxNextExecutionDelay", &PyStreamManipulatorOutputData::MaxNextExecutionDelay)
		.def_readwrite("LastConsumedSampleIDsPerInputStream", &PyStreamManipulatorOutputData::LastConsumedSampleIDsPerInputStream);
}

namespace DynExpModule
{
	void PyStreamManipulatorInputData::Reset()
	{
		ModuleID = DynExp::ItemIDNotSet;
		LastExecutionTime = {};

		InputStreams.clear();
		OutputStreams.clear();
	}

	void PyStreamManipulatorOutputData::Reset()
	{
		MinNextExecutionDelay = {};
		MaxNextExecutionDelay = {};

		LastConsumedSampleIDsPerInputStream.clear();
	}

	void StreamManipulatorData::ResetImpl(dispatch_tag<ModuleDataBase>)
	{
		Init();
	}

	void StreamManipulatorData::Init()
	{
	}

	Util::DynExpErrorCodes::DynExpErrorCodes StreamManipulator::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<StreamManipulator>(Instance.ModuleDataGetter());

			bool IsNewDataAvlbl = false;
			for (size_t i = 0; i < ManipulatorPyFuncInput.InputStreams.size(); ++i)
			{
				auto& Instrument = ModuleData->GetInputDataStreams()[i];
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
				auto SampleStream = InstrData->GetCastSampleStream<SampleStreamType>();

				Instrument->ReadData();

				if (ManipulatorPyFuncOutput.LastConsumedSampleIDsPerInputStream.size() > i &&
					SampleStream->GetNumRecentBasicSamples(ManipulatorPyFuncOutput.LastConsumedSampleIDsPerInputStream[i]))
					IsNewDataAvlbl = true;
			}

			const auto now = std::chrono::system_clock::now();
			if ((ManipulatorPyFuncOutput.MaxNextExecutionDelay.count() && now - LastManipulatorPyFuncExecution >= ManipulatorPyFuncOutput.MaxNextExecutionDelay) ||
				(IsNewDataAvlbl && now - LastManipulatorPyFuncExecution >= ManipulatorPyFuncOutput.MinNextExecutionDelay) ||
				!LastManipulatorPyFuncExecution.time_since_epoch().count())
				Step(ModuleData);

			NumFailedUpdateAttempts = 0;
		} // ModuleData and instruments' data unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void StreamManipulator::ResetImpl(dispatch_tag<ModuleBase>)
	{
		IsInitialized = false;

		ManipulatorPyFuncName = "StreamManipFunc_" + Util::ToStr(GetID());
		ManipulatorPyFuncInit.Reset();
		ManipulatorPyFuncStep.Reset();
		ManipulatorPyFuncExit.Reset();
		ManipulatorPyFuncInput.Reset();
		ManipulatorPyFuncOutput.Reset();

		NumFailedUpdateAttempts = 0;
		LastManipulatorPyFuncExecution = {};
	}

	void StreamManipulator::Step(Util::SynchronizedPointer<ModuleDataType>& ModuleData)
	{
		for (size_t i = 0; i < ManipulatorPyFuncInput.InputStreams.size(); ++i)
		{
			auto& Instrument = ModuleData->GetInputDataStreams()[i];
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
			auto SampleStream = InstrData->GetCastSampleStream<SampleStreamType>();

			ManipulatorPyFuncInput.InputStreams[i].StreamSizeRead = SampleStream->GetStreamSizeRead();
			ManipulatorPyFuncInput.InputStreams[i].StreamSizeWrite = SampleStream->GetStreamSizeWrite();
			ManipulatorPyFuncInput.InputStreams[i].NumSamplesWritten = SampleStream->GetNumSamplesWritten();
			ManipulatorPyFuncInput.InputStreams[i].Samples = SampleStream->ReadRecentBasicSamples(ManipulatorPyFuncOutput.LastConsumedSampleIDsPerInputStream[i]);
		}
		for (size_t i = 0; i < ManipulatorPyFuncInput.OutputStreams.size(); ++i)
		{
			auto& Instrument = ModuleData->GetOutputDataStreams()[i];
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
			auto SampleStream = InstrData->GetCastSampleStream<SampleStreamType>();

			ManipulatorPyFuncInput.OutputStreams[i].StreamSizeRead = SampleStream->GetStreamSizeRead();
			ManipulatorPyFuncInput.OutputStreams[i].StreamSizeWrite = SampleStream->GetStreamSizeWrite();
			ManipulatorPyFuncInput.OutputStreams[i].NumSamplesWritten = SampleStream->GetNumSamplesWritten();
			ManipulatorPyFuncInput.OutputStreams[i].Samples.clear();
		}

		ManipulatorPyFuncInput.LastExecutionTime = LastManipulatorPyFuncExecution;

		PyStreamManipulatorOutputData FuncOutput;
		{
			py::gil_scoped_acquire acquire;

			auto PyResult = ManipulatorPyFuncStep(&ManipulatorPyFuncInput);
			if (!PyResult.is_none())
				FuncOutput = PyResult.cast<PyStreamManipulatorOutputData>();
		} // GIL released here.
		
		LastManipulatorPyFuncExecution = std::chrono::system_clock::now();
		ManipulatorPyFuncOutput.MinNextExecutionDelay = FuncOutput.MinNextExecutionDelay;
		ManipulatorPyFuncOutput.MaxNextExecutionDelay = FuncOutput.MaxNextExecutionDelay;

		for (size_t i = 0; i < ManipulatorPyFuncInput.InputStreams.size(); ++i)
		{
			auto& Instrument = ModuleData->GetInputDataStreams()[i];
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
			auto SampleStream = InstrData->GetCastSampleStream<SampleStreamType>();

			if (FuncOutput.LastConsumedSampleIDsPerInputStream.size() > i
				&& FuncOutput.LastConsumedSampleIDsPerInputStream[i] < SampleStream->GetNumSamplesWritten()
				&& FuncOutput.LastConsumedSampleIDsPerInputStream[i] >= SampleStream->GetNumSamplesWritten() - SampleStream->GetStreamSizeWrite())
				ManipulatorPyFuncOutput.LastConsumedSampleIDsPerInputStream[i] = FuncOutput.LastConsumedSampleIDsPerInputStream[i];
			else
				ManipulatorPyFuncOutput.LastConsumedSampleIDsPerInputStream[i] = SampleStream->GetNumSamplesWritten();
		}
		for (size_t i = 0; i < ManipulatorPyFuncInput.OutputStreams.size(); ++i)
		{
			auto& Instrument = ModuleData->GetOutputDataStreams()[i];
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
			auto SampleStream = InstrData->GetCastSampleStream<SampleStreamType>();

			if (!ManipulatorPyFuncInput.OutputStreams[i].Samples.empty())
			{
				SampleStream->WriteBasicSamples(ManipulatorPyFuncInput.OutputStreams[i].Samples);
				Instrument->WriteData();
			}
		}
	}

	void StreamManipulator::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<StreamManipulator>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<StreamManipulator>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->InputDataStreams, ModuleData->GetInputDataStreams());
		Instance->LockObject(ModuleParams->OutputDataStreams, ModuleData->GetOutputDataStreams());

		auto PythonCode = Util::ReadFromFile(ModuleParams->PythonCodePath.GetPath());
		PythonCode = std::regex_replace(PythonCode, std::regex("\r\n"), "\n");
		PythonCode = std::regex_replace(PythonCode, std::regex("\n"), std::string("\n") + Util::PyTab);

		py::gil_scoped_acquire acquire;
		DynExpInstr::PyDataStreamInstrument::import();
		py::exec("import PyModuleStreamManipulator as StreamManipulator");
		py::exec("def " + ManipulatorPyFuncName + "():\n" +
			Util::PyTab + PythonCode + "\n" +
			Util::PyTab + "if 'on_init' in locals() and callable(on_init):\n" +
			Util::PyTab + Util::PyTab + ManipulatorPyFuncName + ".init = on_init\n" +
			Util::PyTab + "if 'on_step' in locals() and callable(on_step):\n" +
			Util::PyTab + Util::PyTab + ManipulatorPyFuncName + ".step = on_step\n" +
			Util::PyTab + "if 'on_exit' in locals() and callable(on_exit):\n" +
			Util::PyTab + Util::PyTab + ManipulatorPyFuncName + ".exit = on_exit");
		auto ManipulatorPyFunc = py::eval(ManipulatorPyFuncName);
		ManipulatorPyFunc();
		ManipulatorPyFuncInit = py::hasattr(ManipulatorPyFunc, "init") ? py::getattr(ManipulatorPyFunc, "init") : py::none();
		ManipulatorPyFuncStep = py::hasattr(ManipulatorPyFunc, "step") ? py::getattr(ManipulatorPyFunc, "step") : py::none();
		ManipulatorPyFuncExit = py::hasattr(ManipulatorPyFunc, "exit") ? py::getattr(ManipulatorPyFunc, "exit") : py::none();

		ManipulatorPyFuncInput.ModuleID = GetID();
		for (size_t i = 0; i < ModuleData->GetInputDataStreams().GetList().size(); ++i)
		{
			auto& Instrument = ModuleData->GetInputDataStreams()[i];
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
			auto SampleStream = InstrData->GetCastSampleStream<SampleStreamType>();

			ManipulatorPyFuncInput.InputStreams.emplace_back(SampleStream->IsBasicSampleTimeUsed(), Instrument->GetValueUnit());
			ManipulatorPyFuncOutput.LastConsumedSampleIDsPerInputStream.push_back(0);
		}
		for (size_t i = 0; i < ModuleData->GetOutputDataStreams().GetList().size(); ++i)
		{
			auto& Instrument = ModuleData->GetOutputDataStreams()[i];
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
			auto SampleStream = InstrData->GetCastSampleStream<SampleStreamType>();

			ManipulatorPyFuncInput.OutputStreams.emplace_back(SampleStream->IsBasicSampleTimeUsed(), Instrument->GetValueUnit());
		}

		ManipulatorPyFuncInit(&ManipulatorPyFuncInput);

		IsInitialized = true;
	}

	void StreamManipulator::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<StreamManipulator>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->GetInputDataStreams());
		Instance->UnlockObject(ModuleData->GetOutputDataStreams());

		try 
		{
			py::gil_scoped_acquire acquire;
			ManipulatorPyFuncExit(&ManipulatorPyFuncInput);
			py::exec("del " + ManipulatorPyFuncName);
		}
		catch (...)
		{
			// Swallow any exception which might arise from the shutdown of the module's Python part
			// since a failure of that is not considered a severe error.
			Util::EventLogger().Log("Shutting down Python part of module \"" + GetObjectName() + "\" failed.", Util::ErrorType::Warning);
		}
	}
}