find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(trackpipe_tests
  test_version.cpp test_frame.cpp)

target_link_libraries(trackpipe_tests
  PRIVATE trackpipe::core trackpipe::project_warnings GTest::gtest_main)

# GoogleTest macros trigger many clang-tidy checks; lint production code only.
set_target_properties(trackpipe_tests PROPERTIES CXX_CLANG_TIDY "")

# One CTest entry per TEST(), so ctest and the VS Code test explorer list them
gtest_discover_tests(trackpipe_tests)
