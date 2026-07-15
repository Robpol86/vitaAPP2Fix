include("${CMAKE_SOURCE_DIR}/cmake/common.cmake" REQUIRED)

# Set variables.
set(MODULE_NAME "${PROJECT_NAME}Test")
set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

# Enable FetchContent feature.
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Download CodeCoverage.
FetchContent_Declare(cmake_modules
  GIT_REPOSITORY  https://github.com/bilke/cmake-modules.git
  GIT_TAG         5b988b5beb64270cf68b7d6c20298ebc8236b580
)
FetchContent_GetProperties(cmake_modules)
if(NOT cmake_modules_POPULATED)
  FetchContent_MakeAvailable(cmake_modules)
endif()
include("${cmake_modules_SOURCE_DIR}/CodeCoverage.cmake" REQUIRED)

# Download cmocka.
FetchContent_Declare(cmocka
  URL                         https://cmocka.org/files/2.0/cmocka-2.0.2.tar.xz
  URL_HASH                    SHA256=39f92f366bdf3f1a02af4da75b4a5c52df6c9f7e736c7d65de13283f9f0ef416
  DOWNLOAD_EXTRACT_TIMESTAMP  TRUE
)
FetchContent_MakeAvailable(cmocka)

# Download fff.
FetchContent_Declare(fff
  GIT_REPOSITORY  https://github.com/meekrosoft/fff.git
  GIT_TAG         5111c61e1ef7848e3afd3550044a8cf4405f4199
)
FetchContent_GetProperties(fff)
if(NOT fff_POPULATED)
  FetchContent_MakeAvailable(fff)
endif()

# Iterate unit test (test_*.c) files.
set(TEST_TARGETS "")
set(TEST_ENVIRONMENT
  "ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1"
  "UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1"
)
file(GLOB_RECURSE TEST_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/module_*/test_*.c")
foreach(test_src_file_abs ${TEST_SRC_FILES})

  # Get relative path, top directory name, and test name.
  file(RELATIVE_PATH test_src_file_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${test_src_file_abs}")  # e.g. module_user/a/test_x.c
  string(REGEX REPLACE "/.*" "" src_module_dir_name "${test_src_file_rel}")  # e.g. module_user
  string(REGEX REPLACE "[^a-zA-Z0-9_]" "__" test_name "${MODULE_NAME}__${test_src_file_rel}")  # e.g. vitaQmBluetoothTest__module_user__a__test_x__c

  # Add test and its dependencies, like the other CMakeLists.txt files.
  add_executable("${test_name}" "${test_src_file_rel}")
  common_compile_definitions("${test_name}")
  common_include_directories("${test_name}")
  target_include_directories("${test_name}" PRIVATE
    "${CMAKE_SOURCE_DIR}/${src_module_dir_name}/src"
  )
  if(src_module_dir_name STREQUAL "module_user")
    target_include_directories("${test_name}" PRIVATE "${VENDOR_DIR}/quickmenureborn")
  endif()
  target_include_directories("${test_name}" PRIVATE "${fff_SOURCE_DIR}")
  target_compile_options("${test_name}" PRIVATE "-idirafter" "$ENV{VITASDK}/arm-vita-eabi/include")
  target_compile_options("${test_name}" PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
  target_link_libraries("${test_name}" PRIVATE cmocka::cmocka)
  add_test(NAME "${test_name}" COMMAND "${test_name}")

  # Add test coverage flags/options.
  append_coverage_compiler_flags_to_target("${test_name}")
  target_link_options("${test_name}" PRIVATE --coverage)
  target_link_options("${test_name}" PRIVATE -fsanitize=address,undefined)
  list(APPEND TEST_TARGETS "${test_name}")

endforeach()
set_tests_properties(${TEST_TARGETS} PROPERTIES ENVIRONMENT "${TEST_ENVIRONMENT}")

# Produce code coverage.
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  list(APPEND GCOVR_ADDITIONAL_ARGS --gcov-executable "llvm-cov gcov")
endif()
setup_target_for_coverage_gcovr_xml(
  NAME coverage
  EXECUTABLE ctest --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure --no-tests=error -V
  DEPENDENCIES ${TEST_TARGETS}
  EXCLUDE
    "${CMAKE_BINARY_DIR}/_deps/*"
    "${CMAKE_SOURCE_DIR}/tests/*"
    "${CMAKE_SOURCE_DIR}/vendor/*"
)
