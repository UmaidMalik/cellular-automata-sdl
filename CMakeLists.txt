cmake_minimum_required(VERSION 3.20)
project(CellularAutomata LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# output dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
  string(TOUPPER ${cfg} CFG)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/bin/${cfg})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/lib/${cfg})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/lib/${cfg})
endforeach()

# SDL3
option(SDL_SHARED "Build shared SDL library" OFF)
option(SDL_STATIC "Build static SDL library" ON)
option(SDL_TEST "Build SDL test programs" OFF)
add_subdirectory(thirdparty/SDL)  # provides SDL3::SDL3-static

# ImGui (SDL_GPU backends)
add_library(imgui STATIC
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/backends/imgui_impl_sdl3.cpp
    thirdparty/imgui/backends/imgui_impl_sdlgpu3.cpp
)
target_include_directories(imgui PUBLIC
    thirdparty/imgui
    thirdparty/imgui/backends
)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# App sources/headers
file(GLOB_RECURSE CA_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE CA_PRIV_HDR CONFIGURE_DEPENDS "src/*.h" "src/*.hpp")
file(GLOB_RECURSE CA_HEADERS CONFIGURE_DEPENDS
  "src/*.h" "src/*.hpp"
  "include/**/*.h" "include/**/*.hpp"
)

add_executable(CellularAutomata ${CA_SOURCES} ${CA_PRIV_HDR} ${CA_HEADERS})

# Your include paths (public if you plan to export headers)
target_include_directories(CellularAutomata
  PUBLIC  ${PROJECT_SOURCE_DIR}/include
  PRIVATE ${PROJECT_SOURCE_DIR}/src
)

# Pretty groups in IDEs
source_group(TREE "${PROJECT_SOURCE_DIR}/src"     FILES ${CA_SOURCES} ${CA_PRIV_HDR})
source_group(TREE "${PROJECT_SOURCE_DIR}/include" FILES ${CA_PUB_HDR})

# 🔗 Link libraries here (not in target_include_directories)
target_link_libraries(CellularAutomata PRIVATE
    imgui
)

if (WIN32)
    target_link_libraries(CellularAutomata PRIVATE imm32 version winmm)
elseif (APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(CellularAutomata PRIVATE ${COCOA_LIBRARY})
elseif (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(CellularAutomata PRIVATE dl Threads::Threads)
endif()

# Stage the checked-in SPIR-V files beside the executable for every build configuration.
add_custom_target(cellular_automata_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:CellularAutomata>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/build/grid_fullscreen_triangle.vert.spv"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/build/grid_sample_rgba8.frag.spv"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/build/update_r1.frag.spv"
        "$<TARGET_FILE_DIR:CellularAutomata>/shaders"
    VERBATIM
)
add_dependencies(CellularAutomata cellular_automata_shaders)
