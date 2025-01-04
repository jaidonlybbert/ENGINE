export PATH=$VULKAN_SDK/bin:$PATH
cd build
cmake -G "Ninja" -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_C_COMPILER=clang ..
cmake --build . --config Release
.\Release\Engine.exe
