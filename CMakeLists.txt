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
add_subdirectory(thirdparty/fmt)

if (EMSCRIPTEN)
    set(IMGUI_RENDER_BACKEND thirdparty/imgui/backends/imgui_impl_opengl3.cpp)
else()
    set(IMGUI_RENDER_BACKEND thirdparty/imgui/backends/imgui_impl_sdlgpu3.cpp)
endif()

# ImGui with the platform's graphics backend
add_library(imgui STATIC
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/backends/imgui_impl_sdl3.cpp
    ${IMGUI_RENDER_BACKEND}
)
target_include_directories(imgui PUBLIC
    thirdparty/imgui
    thirdparty/imgui/backends
)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)
if (EMSCRIPTEN)
    target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_ES3)
endif()

# App sources/headers
file(GLOB_RECURSE CA_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
if (EMSCRIPTEN)
    list(FILTER CA_SOURCES EXCLUDE REGEX "/(automata_gpu|shader_loader)[.]cpp$")
else()
    list(FILTER CA_SOURCES EXCLUDE REGEX "/web_automata[.]cpp$")
endif()
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
    fmt::fmt
)

if (EMSCRIPTEN)
    function(read_web_shader shader variable)
        set(path "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${path}")
        file(READ "${path}" source)
        string(REPLACE "#version 450" "#version 300 es\nprecision highp float;\nprecision highp int;" source "${source}")
        set(${variable} "${source}" PARENT_SCOPE)
    endfunction()
    read_web_shader(grid_fullscreen_triangle.vert WEB_VERTEX)
    read_web_shader(update_r1.frag WEB_UPDATE)
    read_web_shader(grid_sample_rgba8.frag WEB_DISPLAY)
    configure_file(web/web_shaders.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/web_shaders.hpp" @ONLY)
    target_include_directories(CellularAutomata PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    set_target_properties(CellularAutomata PROPERTIES
        OUTPUT_NAME index
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/site"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/site"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/site"
    )
    set(shell "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
    set_property(TARGET CellularAutomata APPEND PROPERTY LINK_DEPENDS "${shell}")
    target_link_options(CellularAutomata PRIVATE
        "-sMIN_WEBGL_VERSION=2" "-sMAX_WEBGL_VERSION=2"
        "-sALLOW_MEMORY_GROWTH=1" "--shell-file" "${shell}"
    )
elseif (WIN32)
    target_link_libraries(CellularAutomata PRIVATE imm32 version winmm)
elseif (APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(CellularAutomata PRIVATE ${COCOA_LIBRARY})
elseif (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(CellularAutomata PRIVATE dl Threads::Threads)
endif()

if (NOT EMSCRIPTEN)
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
endif()
