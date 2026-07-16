# This file is part of DynExp.

target_sources(DynExpModules PRIVATE
	"SpectrumViewer.cpp"
	"SpectrumViewer.h"
	"SpectrumViewerEvents.cpp"
	"SpectrumViewerEvents.h"
)
qt_add_qml_module(SpectrumViewerModule URI Modules.SpectrumViewer
	VERSION 1.0 STATIC
	QML_FILES "SpectrumViewer.qml"
	SOURCES "SpectrumViewerBackend.cpp" "SpectrumViewerBackend.h"
	IMPORTS TARGET DynExpQuick
)
target_link_libraries(SpectrumViewerModule PRIVATE DynExpConfig)