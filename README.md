[![Ubuntu-GCC](https://github.com/jaidonlybbert/ENGINE/actions/workflows/ubuntu-gcc.yml/badge.svg?branch=main)](https://github.com/jaidonlybbert/ENGINE/actions/workflows/ubuntu-gcc.yml)
# ENGINE
Cross-platform real-time 3D rendering *application template* based on Vulkan

# What is it?
Minimal application to load some gltf data, and render it, with some light abstraction and code organization

# Motivation
This code is for my own benefit to use as a baseline for application development

# Similar/Related Projects
* [Vulkan-Samples](https://github.com/KhronosGroup/Vulkan-Samples) 
* [VulkanSceneGraph](https://github.com/vsg-dev/VulkanSceneGraph) 
* [VulkanTutorial](https://github.com/Overv/VulkanTutorial) 

# Features
- GLTF loading
- PBR rendering
- Scene graph
- KB+M Input
- ImGUI integration

# Supported Platforms
- arm64-osx
- x64-windows

# Recommended Build Instructions
There are many ways you can build this, at the end of the day it's a CMake project. I've chosen to use conda, python, and conan to manage dependencies for cross-platform support.
If everything works, a window should pop up with a render of a default .obj mesh

## All platforms (macOS, Windows, Ubuntu)
### Install the Vulkan SDK
https://www.lunarg.com/vulkan-sdk/

Check that the environment variable VULKAN_SDK is defined and points to the SDK directory
```bash
echo ${VULKAN_SDK}
```
or (Windows)
```CMD
echo %VULKAN_SDK%
```

You can create a CMakeUserPresets.json at the project root to set this variable, or define it another way. The environment variables are copied to the python buildfile.py script, so it has to be defined in the environment where you call that script.

### Install miniconda
https://www.anaconda.com/docs/getting-started/miniconda/install

### Create a new conda environment with the system dependencies, and activate it
```bash
conda create -n Engine cmake conan -c conda-forge
conda activate Engine
```

## Windows specific
### Install Microsoft Visual Studio 17 Community 2022
https://visualstudio.microsoft.com/vs/community/

### Execute the buildfile.py script
```bash
python buildfile.py --preset=mac-default --buildtype=Debug
```

### Open the Visual Studio solution and run
The solution file (.sln) should be located in the "build" folder. In the "solution explorer" right-click on the "Engine" project and "Set as Startup Project" - then hit the big green play button. 

## macOS specific
### Install Xcode command-line tools
```bash
xcode-select --install
```

### Create a CMake preset pointing to the Vulkan SDK (and MoltenVK)
In the project root (same level as CMakeLists.txt)
Create a file named "CMakeUserPresets.txt" and paste the following:
....

### Execute the buildfile.py script
```bash
python buildfile.py --preset=mac-default --buildtype=Debug
```

### Run the executable
```bash
./build/Debug/Engine
```
