cmake_minimum_required(VERSION 3.25)

project(trackpipe
  VERSION 0.1.0
  DESCRIPTION "Real-time C++ perception pipeline"
  LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Options (set them from CMakePresets.json, not by editing this file)
# ---------------------------------------------------------------------------
option(TRACKPIPE_BUILD_TESTS          "Build unit tests"                      ON)
option(TRACKPIPE_BUILD_BENCHMARKS     "Build micro-benchmarks (Phase 2)"      OFF)
option(TRACKPIPE_WARNINGS_AS_ERRORS   "Treat compiler warnings as errors"     OFF)
option(TRACKPIPE_ENABLE_CLANG_TIDY    "Run clang-tidy while compiling"        OFF)
option(TRACKPIPE_ENABLE_CUDA          "Build CUDA/TensorRT parts (Phase 3)"   OFF)
set(TRACKPIPE_SANITIZER "" CACHE STRING
    "Sanitizers to enable: empty, 'address;undefined' or 'thread'")

# C++23 for std::expected; everything else in the plan is C++20.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# compile_commands.json for clangd and clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(ProjectOptions)   # warnings, sanitizers, ccache, clang-tidy

if(TRACKPIPE_ENABLE_CUDA)
  enable_language(CUDA)
endif()

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
add_subdirectory(src)     # trackpipe_core library
add_subdirectory(app)     # trackpipe executable

if(TRACKPIPE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
