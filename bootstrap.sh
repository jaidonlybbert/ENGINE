cmake -G "Ninja" -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_C_COMPILER=clang .
cmake --build .\build --config Release
.\build\Release\Engine.exe
