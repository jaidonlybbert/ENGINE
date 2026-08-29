[![Ubuntu-GCC](https://github.com/jaidonlybbert/ENGINE/actions/workflows/ubuntu-gcc.yml/badge.svg?branch=main)](https://github.com/jaidonlybbert/ENGINE/actions/workflows/ubuntu-gcc.yml)
[![Code Style Check](https://github.com/jaidonlybbert/ENGINE/actions/workflows/style.yml/badge.svg)](https://github.com/jaidonlybbert/ENGINE/actions/workflows/style.yml)
# ENGINE
Cross-platform real-time 3D rendering application template based on Vulkan

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

# Tested Compilers & Platforms
These are the platforms I routinely build on and actively maintain
- arm64-appleclang-osx
- x86_64-msvc-windows
- x86_64-gcc-ubuntu

# Recommended Build Instructions
There are many ways you can build this, at the end of the day it's a Conan + CMake project. I've captured platform specific instructions and system dependencies as best I can.

After you have installed all the system dependencies there are three things you need to do to compile the code for any new build type (Release, Debug, RelWithDebInfo, MinSizeRel)
1. conan install
2. cmake configure
3. cmake build

To make it easier to run those commands, I wrote a `buildfile.py` script that takes two optional arguments: `preset` and `buildtype` which correspond to CMake presets and buildtypes. By default, the CMake preset used is defined in the CMakePresets.json file, you will notice that it inherits from the conan-default preset that is generated when you run `conan install`. To run with other compilers, you will need to define your own preset in a CMakeUserPresets.json file to override the default, and pass that preset into the `buildfile.py` script as `--preset=<your_preset>`

## Install System Dependencies
### Ubuntu (with GCC)
```bash
sudo add-apt-repository universe
sudo add-apt-repository ppa:ubuntu-toolchain-r/ppa -y
sudo apt-get update
sudo apt-get install libwayland-dev xorg-dev gcc-14 cmake
```

### macOS (with AppleClang)
```bash
xcode-select --install
```

### Windows (with MSVC)
#### Install Microsoft Visual Studio 17 Community 2022
https://visualstudio.microsoft.com/vs/community/

#### Install Python 3.10 or later
https://www.python.org/downloads/windows/

#### Install CMake
https://cmake.org/download/

### All platforms (macOS, Windows, Ubuntu)
#### Install the Vulkan SDK
https://www.lunarg.com/vulkan-sdk/

Check that the environment variable VULKAN_SDK is defined and points to the SDK directory
```bash
echo ${VULKAN_SDK}
```
or (Windows)
```CMD
echo %VULKAN_SDK%
```

If it's not defined after installation, you can create a [CMakeUserPresets.json](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) at the project root to set this variable, or define it another way. The environment variables are copied to the python buildfile.py script, so it has to be defined in the environment where you call that script.

#### Clone this repo
```bash
git clone --recurse-submodules https://github.com/jaidonlybbert/ENGINE.git
```

#### Install Conan
```bash
python -m install conan
```

#### Generate a default conan profile for your system
```bash
conan profile detect --force
```

## Build Project

### Ubuntu (GCC)
#### Execute the buildfile.py script
```bash
python buildfile.py --buildtype=Debug
```

#### Run executable
```bash
./build/Debug/Engine
```

### Windows (MSVC)
#### Execute the buildfile.py script
```bash
python buildfile.py --buildtype=Debug
```

#### Open the Visual Studio solution and run
The solution file (.sln) should be located in the "build" folder. After it opens, in the "solution explorer" right-click on the "Engine" project and "Set as Startup Project" - then hit the big green play button. 

## macOS specific
### Create a CMake preset pointing to the Vulkan SDK (and MoltenVK)
In the project root (same level as CMakeLists.txt), create a file named "CMakeUserPresets.json" and paste the following, but change the VULKAN_SDK variable to point to your SDK installation path. Note in some SDK distributions the subdirectory for VK_ICD_FILENAMES and VK_LAYER_PATH is different, and you may have to fix the pathing.
```json
{
  "version": 10,
  "cmakeMinimumRequired": {
    "major": 4,
    "minor": 1,
    "patch": 1
  },
  "include": [
    "CMakePresets.json"
  ],
  "configurePresets": [
    {
      "name": "mac-default",
      "inherits": "default",
      "generator": "Ninja",
      "environment" : {
        "VULKAN_SDK": "$env{HOME}/VulkanSDK/1.3.283.0/macOS",
        "PATH": "$env{VULKAN_SDK}/bin:$penv{PATH}",
        "DYLD_LIBRARY_PATH": "$env{VULKAN_SDK}/lib",
        "VK_ICD_FILENAMES": "$env{VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json",
        "VK_LAYER_PATH": "$env{VULKAN_SDK}/share/vulkan/explicit_layer.d"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "mac-default",
      "configurePreset": "mac-default",
      "description": "Build using mac-default preset",
      "jobs": 8
    }  
  ]
}
```

### Execute the buildfile.py script
```bash
python buildfile.py --preset=mac-default --buildtype=Debug
```

### Run the executable
```bash
./build/Debug/Engine
```
