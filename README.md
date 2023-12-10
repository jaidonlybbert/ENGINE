# ENGINE

# What is it then?
ENGINE is simply a collection of cross-platform C++ graphics libraries along with some build files to kickstart a graphics app for Windows, iOS, MacOS, Linux, and Android.

It is intended to be forked to start simple games and simulations, but does not (and never will) contain any of it's own runtime code.

# Installing
## Pre-requisites
- CMAKE 3.22 or newer
- Vulkan SDK

## Windows

Install the Vulkan SDK https://www.lunarg.com/vulkan-sdk/

git --recurse-submodules clone git@github.com:jaidonlybbert/ENGINE.git

From the /Engine/vcpkg directory run

./bootstrap-vpkg.bat

./vcpkg.exe install glfw3

From /Engine/build run

cmake ..
cmake --build .
