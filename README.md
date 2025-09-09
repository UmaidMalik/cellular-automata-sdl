Windows
cmake --build build --config Debug
cmake --build build --config Release

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build


cmake -S . -B build -G "Visual Studio 17 2022" -A x64


Linux
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-linux
