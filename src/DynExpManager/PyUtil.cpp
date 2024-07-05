// This file is part of DynExp.

#include "stdafx.h"
#include "PyUtil.h"

/**
 * @internal 
*/
PYBIND11_EMBEDDED_MODULE(PyModuleStdoutLogger, m)
{
	py::class_<Util::PyStdoutLoggerWrapper>(m, "StdoutLoggerWrapper")
		.def("write", &Util::PyStdoutLoggerWrapper::write)
		.def("flush", &Util::PyStdoutLoggerWrapper::flush);
}

namespace Util
{
	void PyStdoutLoggerWrapper::write(std::string Str)
	{
		std::erase(Str, '\r');
		std::replace(Str.begin(), Str.end(), '\n', ' ');

		if (!Str.empty() && Str.find_first_not_of(" ") != std::string::npos)
			Util::EventLog().Log("[Py] \"" + Str + "\"");
	}

	PyGilReleasedInterpreter::PyGilReleasedInterpreter()
	{
		py::gil_scoped_acquire acquire;

		// Redirect Python's stdout.
		py::module_::import("PyModuleStdoutLogger");
		Module_sys = py::module_::import("sys");
		Module_sys.attr("stdout") = Logger;

#ifdef DYNEXP_UNIX
		// Add folder 'lib-dynload' to path and use release modules. Otherwise, Python does not find basic modules.
		py::exec(R"(
			new_path = []
			for p in sys.path:
				p = p.replace('debug/', '')
				new_path.append(p)
				if '/vendor/vcpkg/installed/' in p and '/site-packages' in p:
					p = p.replace('site-packages', 'lib-dynload')
					new_path.append(p)
			sys.path = new_path
		)");
#endif // DYNEXP_UNIX
	}

	void PyGilReleasedInterpreter::PrintDebugInfo()
	{
		py::gil_scoped_acquire acquire;

		py::print("Importing Python modules from");
		py::print(Module_sys.attr("path"));
	}
}