# DynExp Installation Quickstart
Compiled [DynExp releases are available on GitHub](https://github.com/jbopp/dynexp/releases).
If you wish to extend or change DynExp or to add support for hardware that uses proprietary third-party libraries, you have to compile DynExp yourself.
This guide summarizes how to compile DynExp in a few steps.
For details and a customized installation refer to the [Readme](./README.md).

## First steps

### Download DynExp 
Use [Git](https://git-scm.com/) to download the DynExp repository.
Under a file path **that does not contain any space character and that is as short as possible**, run
```bash
git clone https://github.com/jbopp/dynexp.git
```

### Compile DynExp and its dependencies
#### Windows
1. Download and install [Visual Studio
](https://visualstudio.microsoft.com/de/) (**not** Visual Studio Code).
2. Run `install_dependencies.ps1` in the folder `src` under the topmost folder of DynExp's repository. This might take several hours and consume about 90 GB disk space.
3. In Visual Studio, [open](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio) the DynExp repository folder.
4. Make sure that you [enable the CMake integration](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio#cmake-partial-activation).
5. Set the file `src\DynExpManager\CMakeLists.txt` as the [startup item](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio#building-cmake-projects) and set the build configuration to `Windows x64 Release` in the Visual Studio toolbar's respective dropdown menus.
6. Press `Ctrl + B` to build DynExp.

#### Linux
1. Make sure that gcc 13 is installed on your system. On Ubuntu 22, for instance, run
	```bash
	sudo add-apt-repository ppa:ubuntu-toolchain-r/test
	sudo apt update
	sudo apt install gcc-13 g++-13
	```
2. Run `install_dependencies.sh` in the folder `src` under the topmost folder of DynExp's repository. This might take several hours and consume about 90 GB disk space.
3. In the folder `src`, call
	```bash
	cmake --preset linux-[debug|relwithdebinfo|release]-[default|user] ./DynExpManager
	```
4. In the folder `out/build/linux-[debug|relwithdebinfo|release]-[default|user]`, call
    ```bash
	ninja
	```

### Run DynExp
You will find the compiled DynExp executable in the folder `out/build/linux-[debug|relwithdebinfo|release]-[default|user]` depending on the chosen build configuration.

## Next steps
Refer to the [Readme](./README.md) to learn how to
* enable compilation with support for devices requiring proprietary third-party libraries (section "Third-party support").
* extend DynExp by adding hardware adapters, instruments or modules (section "Extending DynExp").