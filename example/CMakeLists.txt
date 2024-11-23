#
# svg2img v. 1.0
# Header-only library for converting svg to raster images (png/jpeg/webp) via browser.
# Required Emscripten/WebAssembly environment.
# SPDX-FileCopyrightText: Copyright © 2024 Anatoly Petrov <petrov.projects@gmail.com>
# SPDX-License-Identifier: MIT
#
# CMake file for building svg2img/example/main.cpp
#
# Produces stand-alone HTML file at svg2img/example/build/index.html with embedded JS and WASM.
# You may open it in a browser to inspect the Svg2Img Demo. No web server is needed.
#
# Tech stack: Emscripten + Dear ImGui + GLFW + OpenGL + stb
#
# List of dependencies:
# 1) Emscripten: An LLVM-to-WebAssembly Compiler
#    - Sources: https://github.com/emscripten-core/emscripten
#    - Installation: https://emscripten.org/docs/getting_started/downloads.html
# 2) Dear ImGui: Bloat-free Graphical User interface for C++ with minimal dependencies
#    - Sources: https://github.com/ocornut/imgui
#    - Installation: not needed; this Makefile will compile sources
# 3) stb: Single-file public domain libraries for C/C++
#    - Sources: https://github.com/nothings/stb
#    - Installation: not needed; this Makefile will compile sources
# *) We also need a GLFW, but Emscripten automatically links against it
#	 if the sUSE_GLFW flag is set.
#
# Assumed all required libs/packages were placed in the same directory as svg2img.
# Example of the proper directory tree (/parent/ name is not specified):
# ../parent/
# ..../svg2img/
# ..../emsdk/
# ..../imgui/
# ..../stb/
#
# If this structure is inappropriate for you, please fix the desired paths
# in the [Third-party dirs] sections.
#
# Usage with CLion
# 1) In Settings->Build, Execution, Deployment->Toolchains create a new Emscripten toolchain.
#    Set the following compiler paths:
#    - C Compiler: {your_path}/emsdk/upstream/emscripten/emcc
#    - C++ Compiler: {your_path}/emsdk/upstream/emscripten/em++
# 2) In Settings->Build, Execution, Deployment->CMake create a new CMake profile.
#    Set the following options:
#    - Toolchain: Emscripten (see step 1)
#    - Generator: Unix Makefiles
#    - CMake options: DCMAKE_TOOLCHAIN_FILE={your_path}/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
#                     DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_WORKS=1 -DCMAKE_C_COMPILER_WORKS=1 -G "Unix Makefiles"
#
# See also: https://github.com/emscripten-core/emscripten/blob/main/cmake/Modules/Platform/Emscripten.cmake

cmake_minimum_required(VERSION 3.29)
project(svg2img)

# [C++ Standard]
set(CMAKE_CXX_STANDARD 20)

# [Project dirs]
set(BUILD_DIR "./build")
set(INCLUDE_DIR "../include")

# [Third-party dirs]
set(THIRD_PARTY "../..")
set(EMSC_DIR "${THIRD_PARTY}/emsdk/upstream/emscripten")
set(IMGUI_DIR "${THIRD_PARTY}/imgui")
set(STB_DIR "${THIRD_PARTY}/stb")

# [Includes]
include_directories(
        ${INCLUDE_DIR}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${STB_DIR})

# [Sources]
add_executable(svg2img
        main.cpp
        ${INCLUDE_DIR}/svg2img.h
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)

# [Flags for compiler and linker]
target_compile_options(svg2img PUBLIC
        "SHELL:
		-Wall \
		-Wformat \
		-O3")
target_link_options(svg2img PUBLIC
        "SHELL:
        -sSINGLE_FILE=1 \
		-sALLOW_MEMORY_GROWTH=1 \
		-sUSE_GLFW=3 \
       	-sFULL_ES3 \
        -sEXPORTED_RUNTIME_METHODS=[ccall] \
        -sEXPORTED_FUNCTIONS=[_main,_malloc,_free]")

# [Target]
# We should specify that BUILD_DIR is placed in the parent directory.
# Otherwise, the BUILD_DIR will be created within the cmake-build.
set_target_properties(svg2img PROPERTIES
        OUTPUT_NAME "index"
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY ../${BUILD_DIR})
