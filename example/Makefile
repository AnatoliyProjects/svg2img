#
# svg2img v. 1.0
# Header-only library for converting svg to raster images (png/jpeg/webp) via browser.
# Required Emscripten/WebAssembly environment.
# SPDX-FileCopyrightText: Copyright © 2024 Anatoly Petrov <petrov.projects@gmail.com>
# SPDX-License-Identifier: MIT
#
# Makefile for building svg2img/example/main.cpp
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
#    - Installation: not needed; this build file will compile sources
# 3) stb: Single-file public domain libraries for C/C++
#    - Sources: https://github.com/nothings/stb
#    - Installation: not needed; this build file will compile sources
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

# ----------------------------------------------------------------------------
# Preamble
# ----------------------------------------------------------------------------

# [Project dirs]
BUILD_DIR = ./build
INCLUDE_DIR = ../include

# [Third-party dirs]
THIRD_PARTY = ../..
EMSC_DIR =  $(THIRD_PARTY)/emsdk/upstream/emscripten
IMGUI_DIR = $(THIRD_PARTY)/imgui
STB_DIR = $(THIRD_PARTY)/stb

# [Target, sources, object files]
TARGET = $(BUILD_DIR)/index.html
SOURCES = main.cpp \
		  $(IMGUI_DIR)/imgui.cpp \
		  $(IMGUI_DIR)/imgui_demo.cpp \
		  $(IMGUI_DIR)/imgui_draw.cpp \
		  $(IMGUI_DIR)/imgui_tables.cpp \
		  $(IMGUI_DIR)/imgui_widgets.cpp \
		  $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
		  $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

# [C++ Standard and compiler]
CPP = c++20
CC = $(EMSC_DIR)/em++

# [Flags for compiler and linker]
CFLAGS = -std=$(CPP) \
		 -Wall \
		 -Wformat \
		 -O3 \
		 -I $(INCLUDE_DIR) \
		 -I $(IMGUI_DIR) \
		 -I $(IMGUI_DIR)/backends \
		 -I $(STB_DIR)

EMFLAGS = -sSINGLE_FILE=1 \
		  -sALLOW_MEMORY_GROWTH=1 \
		  -sUSE_GLFW=3 \
		  -sFULL_ES3 \
          -sEXPORTED_RUNTIME_METHODS=[ccall] \
          -sEXPORTED_FUNCTIONS=[_main,_malloc,_free]

# ----------------------------------------------------------------------------
# Rules
# ----------------------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CC) $(CFLAGS) $(EMFLAGS) $(OBJS) -o $(BUILD_DIR)/index.html

$(BUILD_DIR):
	mkdir $@

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: $(IMGUI_DIR)/%.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: $(IMGUI_DIR)/backends/%.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -rf $(OBJS) $(BUILD_DIR)
