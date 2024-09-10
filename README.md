# Archived: vulkan-samples is more supported and does essentially the goal of this project

# ENGINE
ENGINE's not Growing, It's Not an Engine

# What is it then?
Minimal code required to build a cross-platform graphics application for the supported platforms using the listed dependencies.

This is NOT a real-time engine or framework, it has NO API. It is a CMakeLists file bundled with vcpkg manager, and instructions for getting a triangle on screen for any of the supported platforms.

# Features
- Builds an application that opens a window and draws a triangle

# Supported Platforms
- arm64-osx
- x64-windows

# Features
## Windows
- Builds an Installation Wizard (NullSoft Installer System through CMAKE)
- Writes/Reads from Windows Registry to locate install path

# Installing
## Pre-requisites
- CMAKE 3.22 or newer
- Ninja
- Clang
- Vulkan SDK

# Additional Dependencies for MacOS
- pkg-config

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

From /Engine/ run

```
mkdir build
cd build
cmake ..
cmake --build . --config Release
cpack --config .\CPackConfig.cmake
```
