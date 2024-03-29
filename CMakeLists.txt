cmake_minimum_required (VERSION 3.21)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Werror -Wextra -Wshadow -Wconversion -Wpedantic")
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/x64)

project ("CMakeProject" VERSION 1.0.0)

include (cmake/UpdateSubmodules.cmake)
include (CTest)

add_subdirectory (includes)
add_subdirectory(http)

add_executable (${PROJECT_NAME}-${PROJECT_VERSION} main.cpp)
target_link_libraries(${PROJECT_NAME}-${PROJECT_VERSION} PUBLIC http)

if (BUILD_TESTING)

  file (GLOB_RECURSE TEST_FILES tests/*.cpp)
  add_executable (${PROJECT_NAME}-${PROJECT_VERSION}-TEST ${TEST_FILES} ${SRC_FILES})

  target_link_libraries (${PROJECT_NAME}-${PROJECT_VERSION}-TEST PRIVATE GTest::gtest_main)
  target_link_libraries (${PROJECT_NAME}-${PROJECT_VERSION}-TEST PUBLIC http)

  include (GoogleTest)
  gtest_discover_tests (${PROJECT_NAME}-${PROJECT_VERSION}-TEST)

endif()
