cmake_minimum_required (VERSION 3.21)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Werror -Wextra -Wshadow -Wconversion -Wpedantic")

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/x64)

project ("CMakeProject" VERSION 1.0.0)

file (GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable (${PROJECT_NAME}-${PROJECT_VERSION} ${SRC_FILES})

include (CTest)
add_subdirectory (includes)

if (BUILD_TESTING)

  file (GLOB_RECURSE TEST_FILES tests/*.cpp)
  add_executable (${PROJECT_NAME}-${PROJECT_VERSION}-TEST ${TEST_FILES})
  target_link_libraries (${PROJECT_NAME}-${PROJECT_VERSION}-TEST PRIVATE GTest::gtest_main)

  include (GoogleTest)
  gtest_discover_tests (${PROJECT_NAME}-${PROJECT_VERSION}-TEST)

endif()
