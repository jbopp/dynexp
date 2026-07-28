# This file is part of DynExp.

target_sources(DynExpModules PRIVATE "SignalPlotter.cpp" "SignalPlotter.h")
qt_add_qml_module(SignalPlotterModule URI Modules.SignalPlotter
	VERSION 1.0 STATIC
	QML_FILES "SignalPlotter.qml"
	SOURCES "SignalPlotterBackend.cpp" "SignalPlotterBackend.h"
	IMPORTS TARGET DynExpQuick
)
target_link_libraries(SignalPlotterModule PRIVATE DynExpConfig)