# trackpipe_core: everything except main(). Phases add sources here
# (or in subdirectories: sources/, queue/, inference/, tracking/, protocol/).
add_library(trackpipe_core
  core/version.cpp)
add_library(trackpipe::core ALIAS trackpipe_core)

target_include_directories(trackpipe_core
  PUBLIC  ${PROJECT_SOURCE_DIR}/include   # public headers: include/trackpipe/...
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})    # private headers live next to sources

target_compile_definitions(trackpipe_core
  PRIVATE TRACKPIPE_VERSION="${PROJECT_VERSION}")

target_link_libraries(trackpipe_core
  PRIVATE trackpipe::project_warnings)
