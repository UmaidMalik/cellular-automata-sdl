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
and survival. The default is Conway's Game of Life (B3/S23), with two cell
states. **Neighborhood** selects an odd square from 3x3 (8 neighbors) to
11x11 (120 neighbors). The center cell is excluded. Birth and survival
settings for larger counts are remembered when you switch to a smaller
neighborhood. Turn on **Aging states** to delay rebirth after a cell dies. **Max aging
stages** sets the number of generations spent aging. Aging cells do not count
as live neighbors. Disabling aging makes them eligible for birth on the next
generation. Rule changes apply on the next generation.

Births and survivors inherit the circular mean hue of their live neighbors.
For example, hues on either side of red blend to red. If the neighbors'
hues cancel, a survivor keeps its hue and a birth uses the first live
neighbor's hue. Cells keep their last color as they age and die. Once dead,
they fade according to **Color retention**, the fraction of brightness kept
per generation. The panel shows both the decimal value and its percentage.
The slider gives finer control near 1, where small changes
make trails last much longer. Set it to 0 for immediate fading on the next
dead generation, or to 1 to keep the trail visible. Fading cells do not count
as living neighbors. **Brush saturation** sets the saturation of newly
painted cells from 25% to 100%. Existing cells keep their saturation when
the slider changes. A birth inherits the average saturation of its live
neighbors.

The grid starts empty and paused at 4x zoom so individual cells are visible.
Hold the left mouse button and drag to paint living cells. **Brush size**
sets the circular brush diameter from 1 to 500 cells on a logarithmic scale.
Hold Shift while scrolling over the grid to resize it. Enable **Scale brush
with zoom** to keep its apparent size roughly constant when zooming. The
brush is still limited to 1–500 cells. Each cell whose center is inside or
on the circle is randomly set alive or dead, with an even chance of either.
**Random hue per
stroke** picks a different color for every drag. Turn it off to use **Brush
hue** instead. Scroll over the grid to zoom around the pointer,
or use **Zoom** to zoom around the center. Hold the right mouse button and
drag to pan. Set zoom to 1x to see the entire grid.

Unpause the simulation or use **Step** to advance one generation. **Reset**
clears the grid and pauses the simulation. Enable **Pause on draw** to stop
automatic generations while painting. Releasing the brush resumes a running
simulation. Use **Speed** to set a target from
1 to 120 generations per second. The actual rate depends on GPU work and how
often the window can present.
