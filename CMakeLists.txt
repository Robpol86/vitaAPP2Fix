cmake_minimum_required(VERSION 3.28)

option(UNIT_TESTING "Build unit tests" OFF)
message(STATUS "CMAKE_BUILD_TYPE:             ${CMAKE_BUILD_TYPE}")
message(STATUS "UNIT_TESTING:                 ${UNIT_TESTING}")

# Block in-source builds.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are forbidden. Build with: `make clean build`")
endif()

# Block non-debug test builds.
if(UNIT_TESTING)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be Debug when UNIT_TESTING is ON")
  endif()
endif()

# Look for VITASDK environment variable.
if(NOT UNIT_TESTING AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK} AND NOT "$ENV{VITASDK}" STREQUAL "")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(vitaQmBluetooth VERSION 1.0.0 LANGUAGES C)

# Set compiler options.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT UNIT_TESTING)
  add_subdirectory(module_kernel)
  add_subdirectory(module_user)
else()
  enable_testing()
  add_subdirectory(tests)
endif()
