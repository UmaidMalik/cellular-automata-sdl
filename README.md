Windows
cmake --build build --config Debug
cmake --build build --config Release

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build


cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug


Linux
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-linux

## Running the simulation

Launch `CellularAutomata` from the build output (`build/bin/Debug` for the
Visual Studio Debug build). CMake copies the SPIR-V shaders into the `shaders`
folder beside the executable. The app requires an SDL GPU backend that
supports SPIR-V, typically Vulkan.

The Simulation panel lets you pause, advance one generation while paused,
toggle edge wrapping or fullscreen, and select the neighbor counts for birth
and survival. Rule changes apply on the next generation.

Living neighbors contribute their hue to the next generation. Dead cells keep
their last hue and fade according to **Color retention**, the fraction of
brightness kept per generation. Set it to 0 for immediate fading on the next
dead generation, or to 1 to keep the trail visible. Fading cells do not count
as living neighbors.
