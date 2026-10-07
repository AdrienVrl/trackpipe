# Shared build settings. Targets opt in to warnings by linking
# trackpipe::project_warnings PRIVATE; sanitizers apply to the whole build.

# --- Warnings --------------------------------------------------------------
add_library(trackpipe_project_warnings INTERFACE)
add_library(trackpipe::project_warnings ALIAS trackpipe_project_warnings)

target_compile_options(trackpipe_project_warnings INTERFACE
  -Wall -Wextra -Wpedantic
  -Wshadow -Wconversion -Wsign-conversion
  -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual
  -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough
  $<$<BOOL:${TRACKPIPE_WARNINGS_AS_ERRORS}>:-Werror>)

# --- Sanitizers -------------------------------------------------------------
if(TRACKPIPE_SANITIZER)
  if("thread" IN_LIST TRACKPIPE_SANITIZER AND "address" IN_LIST TRACKPIPE_SANITIZER)
    message(FATAL_ERROR "ThreadSanitizer cannot be combined with AddressSanitizer")
  endif()
  list(JOIN TRACKPIPE_SANITIZER "," _san)
  message(STATUS "Sanitizers: ${_san}")
  # -fno-sanitize-recover makes UBSan findings fail the test instead of just printing
  add_compile_options(-fsanitize=${_san} -fno-sanitize-recover=all -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${_san})
endif()

# --- ccache ------------------------------------------------------------------
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# --- clang-tidy ---------------------------------------------------------------
if(TRACKPIPE_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_PROGRAM clang-tidy REQUIRED)
  # Checks come from .clang-tidy at the repo root; in CI findings become errors
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PROGRAM}")
  if(TRACKPIPE_WARNINGS_AS_ERRORS)
    list(APPEND CMAKE_CXX_CLANG_TIDY "--warnings-as-errors=*")
  endif()
endif()
