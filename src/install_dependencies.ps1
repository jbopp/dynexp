# Parse command-line arguments
param(
    [switch]${skip-install-qt}
)

# Clone vcpkg
$LIB_DIR = "..\vendor"
if (-not (Test-Path -Path $LIB_DIR -PathType Container)) {
    New-Item -ItemType directory -Path $LIB_DIR
    git clone https://github.com/microsoft/vcpkg.git "$LIB_DIR\vcpkg"
    & "$LIB_DIR\vcpkg\bootstrap-vcpkg.bat" -disableMetrics
} else {
    Write-Output "$LIB_DIR already exists. Skipping vcpkg installation."
}

# Install DynExp's dependencies using vcpkg
$cmd = ""

if (-not ${skip-install-qt}) {
    $cmd += "& `"$LIB_DIR\vcpkg\vcpkg`" install qtbase:x64-windows; "
    $cmd += "& `"$LIB_DIR\vcpkg\vcpkg`" install qtserialport:x64-windows; "
    $cmd += "& `"$LIB_DIR\vcpkg\vcpkg`" install qtcharts:x64-windows; "
    $cmd += "& `"$LIB_DIR\vcpkg\vcpkg`" install qtdatavis3d:x64-windows; "
    $cmd += "& `"$LIB_DIR\vcpkg\vcpkg`" install qtsvg:x64-windows; "
}

$cmd += "& `"$LIB_DIR\vcpkg\vcpkg`" install gsl:x64-windows; "
$cmd += "& `"$LIB_DIR\vcpkg\vcpkg`" install grpc:x64-windows; "
$cmd += "& `"$LIB_DIR\vcpkg\vcpkg`" install python3:x64-windows; "
$cmd += "& `"$LIB_DIR\vcpkg\vcpkg`" install pybind11:x64-windows"

try {
    Invoke-Expression $cmd
} catch {
    echo("")
    Write-Error "*** Building a dependency failed."
    exit 1
}

$BUILD_DIR = "..\out"
if (-not (Test-Path -Path $BUILD_DIR -PathType Container)) {
    New-Item -ItemType directory -Path $BUILD_DIR
}

echo("")
echo("*** Installed all dependencies successfully.")
echo("Open the folder .\DynExpManager in Visual Studio Community and compile DynExp there.")
echo("Alternatively, make sure that all build tools (ninja.exe, compiler,...) are available in the path environment variable and call")
echo("cmake -DCMAKE_BUILD_TYPE=[Debug|Release] -B `"..\out\build\[Debug|Release]\`" -G `"Ninja`" -DCMAKE_CXX_COMPILER=[compiler].exe .\DynExpManager")
echo("Subsequently, run 'ninja' in folder `"..\out\build\[Debug|Release]\`"")

exit 0