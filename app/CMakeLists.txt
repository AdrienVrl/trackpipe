add_executable(trackpipe main.cpp)

target_link_libraries(trackpipe
  PRIVATE trackpipe::core trackpipe::project_warnings)
