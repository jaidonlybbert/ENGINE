SET PATH=%VULKAN_SDK%\bin:%PATH%
cd build
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release
.\Release\Engine.exe
cd ..
