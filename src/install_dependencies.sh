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
sudo apt-get install git curl zip unzip tar build-essential ninja-build cmake

# Clone vcpkg
LIB_DIR="../vendor"
if [ ! -d "$LIB_DIR" ]; then
    mkdir "$LIB_DIR"
    git clone https://github.com/microsoft/vcpkg.git "$LIB_DIR/vcpkg"
    "$LIB_DIR/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
else
    echo "$LIB_DIR already exists. Skipping vcpkg installation."
fi

# Required to build qtbase and its dependencies.
# Tested with Qt 6.4.2 on a clean Ubuntu 22.04.1 LTS.
sudo apt-get install pkg-config bison python3-distutils python3-jinja2 autoconf autoconf-archive \
    libtool libgl-dev libegl-dev libinput-dev libfontconfig1-dev \
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
    cmd+="\"$LIB_DIR/vcpkg/vcpkg\" install qtbase:x64-linux && "
    cmd+="\"$LIB_DIR/vcpkg/vcpkg\" install qtserialport:x64-linux && "
    cmd+="\"$LIB_DIR/vcpkg/vcpkg\" install qtcharts:x64-linux && "
    cmd+="\"$LIB_DIR/vcpkg/vcpkg\" install qtdatavis3d:x64-linux && "
    cmd+="\"$LIB_DIR/vcpkg/vcpkg\" install qtsvg:x64-linux && "
fi

cmd+="\"$LIB_DIR/vcpkg/vcpkg\" install gsl:x64-linux && "
cmd+="\"$LIB_DIR/vcpkg/vcpkg\" install grpc:x64-linux && "
cmd+="\"$LIB_DIR/vcpkg/vcpkg\" install python3:x64-linux && "
cmd+="\"$LIB_DIR/vcpkg/vcpkg\" install pybind11:x64-linux"

eval $cmd
if [ $? -ne 0 ]; then
    echo ""
    echo "*** Building a dependency failed."
    exit 1
else
    echo ""
    echo "*** Compiled all dependencies successfully."
    echo "Compile DynExp with:"
    echo "cmake --preset linux-[debug|relwithdebinfo|release]-[default|user] ./DynExpManager"
    echo "Subsequently, run 'ninja' in folder \"../out/build/linux-[debug|relwithdebinfo|release]-[default|user]\""
fi

exit 0