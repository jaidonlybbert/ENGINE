# ENGINE

# What is it then?
ENGINE is simply a collection of cross-platform C++ libraries along with some build files to kickstart a graphics app for Windows and Linux.

It is intended to be forked to start simple games and simulations, but minimizes its own runtime code.

# Features
## Windows
- Builds an Installation Wizard (NullSoft Installer System through CMAKE)
- Writes/Reads from Windows Registry to locate install path

# Installing
## Pre-requisites
- CMAKE 3.22 or newer
- Vulkan SDK

## Windows

Install the Vulkan SDK https://www.lunarg.com/vulkan-sdk/

Clone project, including VCPKG manager submodule.
```
git --recurse-submodules clone git@github.com:jaidonlybbert/ENGINE.git
```

Use VCPKG manager to install dependencies.

From the /Engine/vcpkg directory run

```
./bootstrap-vpkg.bat

./vcpkg.exe install glfw3 winreg
```

Build installation wizard and executables for the template application.

From /Engine/build run

```
cmake ..
cmake --build . --config Release
cpack --config .\CPackConfig.cmake
```
