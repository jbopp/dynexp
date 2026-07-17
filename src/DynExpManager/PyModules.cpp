// This file is part of DynExp.

#include "stdafx.h"
#include "PyModules.h"

PYBIND11_MAKE_OPAQUE(decltype(DynExpInstr::PyDataStreamInstrument::Samples));

/**
 * @brief Creates an embedded Python module @p PyModuleDataStreamInstrument
 * which contains DynExpInstr::BasicSample, DynExp::Units::UnitType,
 * DynExpInstr::PyDataStreamInstrument::Samples, and DynExpInstr::PyDataStreamInstrument
 * as respective Python classes.
*/
PYBIND11_EMBEDDED_MODULE(PyModuleDataStreamInstrument, m)
{
	using namespace DynExpInstr;

	py::class_<BasicSample>(m, "BasicSample")
		.def(py::init<>())
		.def(py::init<typename BasicSample::DataType>())
		.def(py::init<typename BasicSample::DataType, typename BasicSample::DataType>())
		.def_readwrite("Value", &BasicSample::Value)
		.def_readwrite("Time", &BasicSample::Time);

	py::enum_<DynExp::Units::UnitType>(m, "UnitType")
		.value("Arbitrary", DynExp::Units::UnitType::Arbitrary)
		.value("LogicLevel", DynExp::Units::UnitType::LogicLevel)
		.value("Counts", DynExp::Units::UnitType::Counts)
		.value("Volt", DynExp::Units::UnitType::Volt)
		.value("Ampere", DynExp::Units::UnitType::Ampere)
		.value("Power_W", DynExp::Units::UnitType::Power_W)
		.value("Power_dBm", DynExp::Units::UnitType::Power_dBm);

	py::bind_vector<decltype(DynExpInstr::PyDataStreamInstrument::Samples)>(m, "SampleListType");

	py::class_<PyDataStreamInstrument>(m, "DataStreamInstrument")
		.def(py::init<>())
		.def("CalcLastConsumedSampleID", &PyDataStreamInstrument::CalcLastConsumedSampleID)
		.def("ConsumeNone", &PyDataStreamInstrument::ConsumeNone)
		.def("ConsumeAll", &PyDataStreamInstrument::ConsumeAll)
		.def("Clear", &PyDataStreamInstrument::Clear)
		.def_readonly("IsTimeUsed", &PyDataStreamInstrument::IsTimeUsed)
		.def_readonly("ValueUnit", &PyDataStreamInstrument::ValueUnit)
		.def_readonly("StreamSizeRead", &PyDataStreamInstrument::StreamSizeRead)
		.def_readonly("StreamSizeWrite", &PyDataStreamInstrument::StreamSizeWrite)
		.def_readonly("NumSamplesWritten", &PyDataStreamInstrument::NumSamplesWritten)
		.def_readwrite("Samples", &PyDataStreamInstrument::Samples);
}

namespace DynExpInstr
{
	void PyDataStreamInstrument::import()
	{
		py::exec("import PyModuleDataStreamInstrument as DataStreamInstrument");
	}

	size_t PyDataStreamInstrument::CalcLastConsumedSampleID(size_t NumConsumedSamples) const
	{
		return NumSamplesWritten - Samples.size() + std::min(Samples.size(), NumConsumedSamples);
	}

	size_t PyDataStreamInstrument::ConsumeAll() const
	{
		return NumSamplesWritten;
	}
}