// This file is part of DynExp.

/**
 * @file PyUtil.h
 * @brief Provides utilities related to pybind11 within %DynExp's %Util namespace.
*/

#pragma once

#include "stdafx.h"

namespace py = pybind11;

namespace Util
{
	/**
	 * @brief Character sequence to indent a Python instruction by one level.
	*/
	static constexpr auto PyTab = "    ";

	/**
	 * @brief Forwards Python's stdout to Util::EventLogger instance defined in Util::EventLog().
	*/
	class PyStdoutLoggerWrapper
	{
	public:
		/**
		 * @brief Formats the string passed to the function and writes it to the event log.
		 * @param Str Message to forward to %DynExp's event log
		*/
		void write(std::string Str);

		/**
		 * @brief Required, but does nothing.
		*/
		void flush() {}
	};

	/**
	 * @brief Initialize Python interpreter and directly release the GIL after construction.
	*/
	class PyGilReleasedInterpreter
	{
	public:
		PyGilReleasedInterpreter();

		void PrintDebugInfo();				//!< Writes information on the Python interpreter configuration to DynExp's log.

	private:
		// Do not change order. GILRelease needs to be destroyed first upon destruction.
		PyStdoutLoggerWrapper Logger;			//!< Wrapper to forward %DynExp's event log to Python
		py::scoped_interpreter Interpreter;		//!< Python interpreter
		py::module_ Module_sys;				//!< Handle to the Python sys module
		py::gil_scoped_release GILRelease;		//!< Release which releases the GIL at construction 
	};

	/**
	 * @brief Wraps a class derived from pybind11::object and ensures that the GIL
	 * is acquired when the PyObject is destroyed. For any other operation
	 * on this wrapper, the GIL still needs to be acquired manually before!
	 * @tparam T Type derived from pybind11::object
	*/
	template <typename T, std::enable_if_t<
		std::is_base_of_v<py::object, T>, int> = 0
	>
	class PyObject
	{
	public:
		using ObjectType = T;

		/**
		 * @brief Constructs an empty PyObject.
		*/
		PyObject() { Reset(); }

		/**
		 * @brief Copy-constructs a PyObject from @p Object.
		 * @param Object Object to copy
		*/
		PyObject(const T& Object) : Object(std::make_unique<T>(Object)) {}

		/**
		 * @brief Removes the owned @p pybind11::object after locking the GIL.
		*/
		~PyObject() { Remove(); }

		/**
		 * @brief Locks the GIL, removes the owned object and copy-assigns from @p Object.
		 * @param Object Object to copy 
		 * @return Reference to this PyObject instance
		*/
		PyObject& operator=(const T& Object)
		{
			this->Object = std::make_unique<T>(Object);
			return *this;
		}

		/**
		 * @brief Returns the wrapped @p pybind11::object
		 * @return Owned object or nullptr if no object is owned
		*/
		auto& Get() const noexcept { return *Object; }

		/**
		 * @copydoc Get() const noexcept
		*/
		auto& Get() noexcept { return *Object; }

		/**
		 * @copydoc ~PyObject()
		 * This PyObject instance does not own an object after this operation.
		*/
		void Reset()
		{
			py::gil_scoped_acquire acquire;
			Object = std::make_unique<T>();
		}

		/**
		 * @brief Calls the wrapped object if it is not nullptr and if the wrapped object
		 * is not pybind11::none.
		 * @tparam ...ArgTs Types of the perfectly forward arguments
		 * @param ...Args Arguments to perfectly forward to the wrapped object's call.
		 * @return Return value of the call of type derived from pybind11::object or
		 * pybind11::none if the call did not happen.
		*/
		template <typename... ArgTs>
		py::object operator()(ArgTs&& ...Args) const
		{
			if (Object && !Object->is_none())
				return (*Object)(std::forward<ArgTs>(Args)...);
			else
				return py::none();
		}

	private:
		/**
		 * @copydoc ~PyObject()
		*/
		void Remove()
		{
			py::gil_scoped_acquire acquire;
			Object.reset();
		}

		/**
		 * @brief Object derived from @p pybind11::object owned by this PyObject instance
		*/
		std::unique_ptr<T> Object;
	};
}