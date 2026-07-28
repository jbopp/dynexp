#!/bin/bash

SKIP_INSTALL_QT=false

# Parse command-line arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --skip-install-qt) SKIP_INSTALL_QT=true ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

# Required by vcpkg and for building
sudo apt-get install git curl zip unzip tar build-essential g++-14 ninja-build cmake python3-venv

# Clone vcpkg
LIB_DIR="../vendor/vcpkg"
if [ ! -d "$LIB_DIR" ]; then
    mkdir -p "$LIB_DIR"
    git clone https://github.com/microsoft/vcpkg.git "$LIB_DIR"
    "$LIB_DIR/bootstrap-vcpkg.sh" -disableMetrics
else
    echo "$LIB_DIR already exists. Skipping vcpkg installation."
fi

# Required to build qtbase and its dependencies.
# Tested with Qt 6.10.2 on a clean Ubuntu 24.04.4 LTS.
sudo apt-get install pkg-config bison flex python3-jinja2 autoconf autoconf-archive \
    libtool libgl-dev libegl-dev libinput-dev libfontconfig1-dev libsm-dev \
    libfreetype6-dev libx11-dev libx11-xcb-dev '^libxcb.*-dev' libxext-dev libxfixes-dev libxkbcommon-dev \
    libxi-dev libxkbcommon-x11-dev libxrender-dev libgl1-mesa-dev libglu1-mesa-dev libegl1-mesa-dev

# Required to build Qt's dependencies on Debian
export ZIC=$(which zic)

# Required by pybind11
sudo apt-get install python3-dev

# Required to link DynExp with gRPC
sudo apt-get install libsystemd-dev

# Install DynExp's dependencies using vcpkg
cmd=""

if [ "$SKIP_INSTALL_QT" = false ]; then
    cmd+="\"$LIB_DIR/vcpkg\" install qtbase:x64-linux && "
    cmd+="\"$LIB_DIR/vcpkg\" install qtgraphs:x64-linux && "
    cmd+="\"$LIB_DIR/vcpkg\" install qtserialport:x64-linux && "
    cmd+="\"$LIB_DIR/vcpkg\" install qtsvg:x64-linux && "
    cmd+="\"$LIB_DIR/vcpkg\" install qtcharts:x64-linux && "
    cmd+="\"$LIB_DIR/vcpkg\" install qtdatavis3d:x64-linux && "
fi

cmd+="\"$LIB_DIR/vcpkg\" install gsl:x64-linux && "
cmd+="\"$LIB_DIR/vcpkg\" install grpc:x64-linux && "
cmd+="\"$LIB_DIR/vcpkg\" install python3:x64-linux && "
cmd+="\"$LIB_DIR/vcpkg\" install pybind11:x64-linux"

eval $cmd
if [ $? -ne 0 ]; then
    echo ""
    echo "*** Building a dependency failed."
    exit 1
else
    echo ""
    echo "*** Compiled all dependencies successfully."
    echo "Compile DynExp with:"
    echo "cmake --preset linux-[debug|relwithdebinfo|release]-[default|user] -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14 ./DynExpManager"
    echo "Subsequently, run 'ninja' in folder \"../out/build/linux-[debug|relwithdebinfo|release]-[default|user]\""
fi

exit 0