# This file is part of DynExp.

target_sources(DynExp PRIVATE "ArbitraryFunctionFromCSV.cpp" "ArbitraryFunctionFromCSV.h")
target_sources(DynExp PRIVATE "gRPCModule.cpp" "gRPCModule.h")
target_sources(DynExp PRIVATE "InputPortReader.cpp" "InputPortReader.h" "InputPortReader.ui")
target_sources(DynExp PRIVATE "LockinAmplifierControl.cpp" "LockinAmplifierControl.h" "LockinAmplifierControl.ui")
target_sources(DynExp PRIVATE "NetworkAnalogInModule.cpp" "NetworkAnalogInModule.h")
target_sources(DynExp PRIVATE "NetworkAnalogOutModule.cpp" "NetworkAnalogOutModule.h")
target_sources(DynExp PRIVATE "NetworkDataStreamInstrumentModule.cpp" "NetworkDataStreamInstrumentModule.h")
target_sources(DynExp PRIVATE "NetworkDigitalInModule.cpp" "NetworkDigitalInModule.h")
target_sources(DynExp PRIVATE "NetworkDigitalOutModule.cpp" "NetworkDigitalOutModule.h")
target_sources(DynExp PRIVATE "NetworkTimeTaggerModule.cpp" "NetworkTimeTaggerModule.h")
target_sources(DynExp PRIVATE "OutputPortWriter.cpp" "OutputPortWriter.h" "OutputPortWriter.ui")
target_sources(DynExp PRIVATE "SignalDesigner.cpp" "SignalDesigner.h" "InputPortReader.ui")
target_sources(DynExp PRIVATE "SignalPlotter.cpp" "SignalPlotter.h" "SignalPlotter.ui")
target_sources(DynExp PRIVATE "Stage1D.cpp" "Stage1D.h" "Stage1D.ui")
target_sources(DynExp PRIVATE "StreamManipulator.cpp" "StreamManipulator.h")
target_sources(DynExp PRIVATE "Trajectory1D.cpp" "Trajectory1D.h" "Trajectory1D.ui")

add_subdirectory(ImageViewer)
add_subdirectory(ODMR)
add_subdirectory(SpectrumViewer)
add_subdirectory(WidefieldMicroscope)