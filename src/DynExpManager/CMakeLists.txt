# This file is part of DynExp.

cmake_minimum_required(VERSION 3.22)

# MSVC debug information format flags are selected by an abstraction (new in version 3.25).
if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
endif()

# Language standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Project configuration
project("DynExp")
set(PROJECT_VERSION 1.0.0)

option(USE_NIDAQ "Compile with third-party NI-DAQmx support" OFF)
option(USE_PVCAM "Compile with third-party PVCam support" OFF)
option(USE_QUTOOLSQUTAG "Compile with third-party Qutools QuTAG support" OFF)
option(USE_SMARACT "Compile with third-party SmarAct support" OFF)
option(USE_SWABIANPULSESTREAMER "Compile with third-party Swabian Instruments Pulse Streamer support" OFF)
option(USE_ZIMFLI "Compile with third-party Zurich Instruments MFLI support" OFF)

# Apply parameters to template files
configure_file(main.cpp.in main.cpp)
configure_file(DynExpDefinitions.h.in DynExpDefinitions.h)

# Additional include directories
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Compiler and linker options
if (MSVC)
	# Warning level 3
	add_compile_options(/W3)

	# Enable unwind semantics
	add_compile_options(/EHsc)

	# Allow large .obj files (otherwise, compiler may complain).
	add_compile_options(/bigobj)
else()
	# lots of warnings
	add_compile_options(-Wall -pedantic)

	# Turn off -Woverloaded-virtual since the warning is triggered heavily by the tag dispatch mechanism.
	add_compile_options(-Wno-overloaded-virtual)

	# Colored error messages
	add_compile_options(-fdiagnostics-color=always)
endif()

# Debug macro
if (CMAKE_BUILD_TYPE MATCHES DEBUG|Debug)
	set(DYNEXP_DEBUG TRUE)
	add_compile_definitions(DYNEXP_DEBUG)
endif()

# OS macros
if (MSVC)
	add_compile_definitions(DYNEXP_MSVC)
elseif (UNIX)
	add_compile_definitions(DYNEXP_UNIX)
endif()

# vcpkg
include("../../vendor/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Disable use of API deprecated in Qt 5.15 and earlier
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

find_package(Qt6 COMPONENTS
	Core
	Gui
	Svg
	Widgets
	Xml
	Network
	REQUIRED
)
find_package(Qt6SerialPort CONFIG REQUIRED)
find_package(Qt6Charts CONFIG REQUIRED)
find_package(Qt6DataVisualization CONFIG REQUIRED)
if (${Qt6_FOUND})
	message("Using Qt " ${Qt6_VERSION})
endif()

# GNU Scientific Library (GSL)
find_package(GSL REQUIRED)

# gRPC
find_package(gRPC CONFIG REQUIRED)

# pybind11
# Fix bug regarding Python3 and pybind11 in debug mode: always use release version of Python.
find_package(Python3 COMPONENTS Interpreter Development)
set_target_properties(Python3::Module PROPERTIES MAP_IMPORTED_CONFIG_DEBUG ";RELEASE")
find_package(pybind11 REQUIRED)

# DynExp target definition
add_executable(DynExp "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")

# Export less symbols in GCC and Clang
set_target_properties(DynExp PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(DynExp PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

# Build for edit and continue
if (MSVC AND CMAKE_BUILD_TYPE MATCHES DEBUG|Debug AND NOT CMAKE_VERSION VERSION_LESS 3.25)
	# Requires CMP0141 set to NEW.
	set_property(TARGET DynExp PROPERTY MSVC_DEBUG_INFORMATION_FORMAT EditAndContinue)
endif()

# Application meta data for Windows
if (MSVC)
	# No console window
	set_property(TARGET DynExp PROPERTY WIN32_EXECUTABLE TRUE)

	# App icon
	target_sources(DynExp PRIVATE "DynExpManager.rc")
endif()

# DynExp's source files
target_precompile_headers(DynExp PRIVATE "stdafx.h")
target_sources(DynExp PRIVATE
	"BusyDialog.cpp"
	"BusyDialog.h"
	"BusyDialog.ui"
	"ChoiceListDialog.cpp"
	"ChoiceListDialog.h"
	"ChoiceListDialog.ui"
	"CircuitDiagram.cpp"
	"CircuitDiagram.h"
	"CircuitDiagram.ui"
	"circularbuf.cpp"
	"circularbuf.h"
	"CommonModuleEvents.cpp"
	"CommonModuleEvents.h"
	"DynExpAbout.cpp"
	"DynExpAbout.h"
	"DynExpAbout.ui"
	"DynExpCore.cpp"
	"DynExpCore.h"
	"DynExpDefinitions.cpp"
	"DynExpDefinitions.h"
	"DynExpManager.cpp"
	"DynExpManager.h"
	"DynExpManager.ui"
	"DynExpManager.qrc"
	"ErrorListDialog.cpp"
	"ErrorListDialog.h"
	"ErrorListDialog.ui"
	"Exception.cpp"
	"Exception.h"
	"HardwareAdapter.cpp"
	"HardwareAdapter.h"
	"Instrument.cpp"
	"Instrument.h"
	"Libraries.cpp"
	"Libraries.h"
	"Managers.cpp"
	"Managers.h"
	"Module.cpp"
	"Module.h"
	"Object.cpp"
	"Object.h"
	"ParamsConfig.cpp"
	"ParamsConfig.h"
	"ParamsConfig.ui"
	"PyModules.cpp"
	"PyModules.h"
	"PythonSyntaxHighlighter.cpp"
	"PythonSyntaxHighlighter.h"
	"PyUtil.cpp"
	"PyUtil.h"
	"QModules.qrc"
	"QtUtil.cpp"
	"QtUtil.h"
	"TextEditor.cpp"
	"TextEditor.h"
	"TextEditor.ui"
	"Util.cpp"
	"Util.h"
)

add_subdirectory(proto)
add_subdirectory(HardwareAdapters)
add_subdirectory(MetaInstruments)
add_subdirectory(Instruments)
add_subdirectory(Modules)

# Generate protobuf files for gRPC
if (PROTO_FILES)
	include("include/grpc_generate_cpp.cmake")
	protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
	grpc_generate_cpp(GRPC_SRCS GRPC_HDRS "." ${PROTO_FILES})
	target_sources(DynExp PRIVATE ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})
endif()

# Libs
list(TRANSFORM EXTRA_LIBS PREPEND "../../../vendor/")
list(TRANSFORM SHARED_LIBS PREPEND "../../../vendor/")
target_link_libraries(DynExp PRIVATE
	Qt::Core
	Qt::Gui
	Qt::Widgets
	Qt::Xml
	Qt::Network
	Qt::SerialPort
	Qt::Charts
	Qt::DataVisualization
	GSL::gsl
	gRPC::grpc++
	Python3::Module
	pybind11::embed
	${EXTRA_LIBS}
)

if (UNIX)
	# Without that, SVG icons are not displayed under Linux
	target_link_libraries(DynExp PRIVATE Qt6::QSvgIconPlugin)

	# Add symbols required by embedded Python
	target_link_options(DynExp PRIVATE -rdynamic)
endif()

if (WIN32)
	if (CMAKE_BUILD_TYPE MATCHES DEBUG|Debug)
		# Copy Qt DLLs to build folder using debug libraries
		string(REGEX REPLACE "\.exe$" "\.debug\.bat" WINDEPLOYQT_DEBUG_EXECUTABLE "${WINDEPLOYQT_EXECUTABLE}")
		if (EXISTS "${WINDEPLOYQT_DEBUG_EXECUTABLE}")
			add_custom_command(TARGET DynExp POST_BUILD COMMAND "${WINDEPLOYQT_DEBUG_EXECUTABLE}" ARGS --no-translations $<TARGET_FILE:DynExp>)
		else()
			add_custom_command(TARGET DynExp POST_BUILD COMMAND Qt::windeployqt ARGS --no-translations $<TARGET_FILE:DynExp>)
		endif()

		# Fix bug regarding zlib in debug mode (depending on configuration, zlib1.dll is also required in debug mode).
		if (${ZLIB_FOUND})
			if (NOT DEFINED ZLIB_RUNTIME_LIBRARY_RELEASE)
				# Replace "lib" only if followed by "/zlib" (not in case of other "lib/" folders).
				string(REGEX REPLACE "/lib/z" "/bin/z" ZLIB_RUNTIME_LIBRARY_RELEASE "${ZLIB_LIBRARY_RELEASE}")
				string(REGEX REPLACE "\.lib$" "1.dll" ZLIB_RUNTIME_LIBRARY_RELEASE "${ZLIB_RUNTIME_LIBRARY_RELEASE}")
			endif()
			add_custom_command(TARGET DynExp POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${ZLIB_RUNTIME_LIBRARY_RELEASE} "${CMAKE_CURRENT_BINARY_DIR}/")
		endif()

		# Fix bug regarding Python3 and pybind11 in debug mode: always use release version of Python.
		add_custom_command(TARGET DynExp POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${Python3_RUNTIME_LIBRARY_RELEASE} "${CMAKE_CURRENT_BINARY_DIR}/")
	else()
		# Copy Qt DLLs to build folder
		add_custom_command(TARGET DynExp POST_BUILD COMMAND Qt::windeployqt ARGS --no-translations $<TARGET_FILE:DynExp>)
	endif()
endif()

# Install pip for Python3 and required modules if missing
execute_process(COMMAND ${Python3_EXECUTABLE} -c "import pip" RESULT_VARIABLE PYTHON_PIP_AVAILABLE OUTPUT_QUIET ERROR_QUIET)
if (NOT ${PYTHON_PIP_AVAILABLE} EQUAL 0)
	message("Installing Python3/pip. This may take some time.")
	execute_process(COMMAND ${Python3_EXECUTABLE} -m ensurepip --upgrade)
endif()

set(Python_REQUIRED_MODULES numpy rpyc simple-pid)
list(APPEND Python_REQUIRED_MODULES ${Python_USER_REQUIRED_MODULES})
foreach (Python_MODULE IN LISTS Python_REQUIRED_MODULES)
	string(REGEX REPLACE "-" "_" Python_MODULE_IMPORT "${Python_MODULE}")
	execute_process(COMMAND ${Python3_EXECUTABLE} -c "import ${Python_MODULE_IMPORT}" RESULT_VARIABLE PYTHON_MODULE_AVAILABLE OUTPUT_QUIET ERROR_QUIET)
	if (NOT ${PYTHON_MODULE_AVAILABLE} EQUAL 0)
		message("Installing Python3/${Python_MODULE}. This may take some time.")
		execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install --no-warn-script-location ${Python_MODULE})
	endif()
endforeach()

# Under Windows, copy Python3 packages to build folder
get_filename_component(Python3_directory ${Python3_EXECUTABLE} DIRECTORY)
if (MSVC)
	add_custom_command(TARGET DynExp POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${Python3_directory}/Lib" "${CMAKE_CURRENT_BINARY_DIR}/Lib")
	add_custom_command(TARGET DynExp POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${Python3_directory}/DLLs" "${CMAKE_CURRENT_BINARY_DIR}/DLLs")
endif()

# Copy dynamic-link libraries to binary folder
if (SHARED_LIBS)
	if (WIN32)
		list(REMOVE_DUPLICATES SHARED_LIBS)
		list(TRANSFORM SHARED_LIBS APPEND ".dll")
		foreach (SHARED_LIB IN LISTS SHARED_LIBS)
			if (EXISTS ${SHARED_LIB})
				add_custom_command(TARGET DynExp POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SHARED_LIB} "${CMAKE_CURRENT_BINARY_DIR}/")
			else()
				message(WARNING "Cannot copy ${SHARED_LIB}. File does not exist.")
			endif()
		endforeach()
	else()
		message("Copying shared third-party libraries automatically to the binary directory is not implemented for platforms other than Windows.")
		message("Please copy the libraries manually if needed.")
	endif()
endif()