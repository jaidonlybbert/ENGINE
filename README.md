# VulkanTemplateApp
Basic setup for a Vulkan application

# What is it?
Minimal code required to build a cross-platform graphics application for the supported platforms using the listed dependencies.

This is NOT an engine or a framework, it has NO API. It is essentially a CMakeLists file to build the [Vulkan-Tutorial](https://vulkan-tutorial.com) for any major platform. The scope is intentionally *very small* so it can be *understood* and *extended*. I will only add features if they are so obvious and necessary that I can't stand re-implementing the same thing over and over across multiple projects. And in that case I will try to keep it as simple as possible and avoid any abstractions.

# Motivation
This code is for my own benefit to use as a baseline for (closed-source for-profit) application development, which may include games, simulations, or data visualization, without having to start from scratch each time. There are many other Vulkan projects out there that attempt to do something similar. I'm making this open-source so I can copy it on behalf of any entity without worrying about licensing issues, not because I want to be an open-source maintainer.

# Similar/Related Projects
* [Vulkan-Samples](https://github.com/KhronosGroup/Vulkan-Samples) includes a framework for Vulkan application development. My experience with this framework was that it was easy to get started, but extremely difficult to navigate all the undocumented abstractions and interactions across directories in order to add any functionality. The heavy use of git subprojects and compilation of all dependencies was also cumbersome IMO.
* [VulkanSceneGraph](https://github.com/vsg-dev/VulkanSceneGraph) is the brain-child of Robert Osfield who is responsible for about 99.4% of the commited code. My experience with this project was that the abstractions weren't documented well enough for me to avoid reading source code to determine behavior. Since the project is distributed as a compiled library, it didn't play nice with my LSP, since the headers didn't include (good) docstrings with the declarations and I couldn't jump to definitions. It could be skill issues on my part, but it just didn't feel good, and I frankly don't have a lot of trust in such an ambitious project effectively maintained by a single person.
* [VulkanTutorial](https://github.com/Overv/VulkanTutorial) is where the bulk of the code in **this** repo comes from. The tutorial code implements the bare essentials for a Vulkan app without many abstractions, and is very well documented since the associated guide explains the code line-by-line. In *this* repo, I have basically lifted the code from the last chapter of *that* repo and made it easy to build as a standalone cross-platform application without the rest of the stuff in that repo.

# Features
- Builds an application that opens a window, loads a textured OBJ, and renders it

# Supported Platforms
- arm64-osx
- x64-windows

# Build Instructions
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
