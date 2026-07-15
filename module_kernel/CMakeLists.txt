include("${CMAKE_SOURCE_DIR}/cmake/common.cmake" REQUIRED)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

# Set variables.
set(EXPORTS_YML "${CMAKE_CURRENT_BINARY_DIR}/cmake_gen/exports.yml")
set(MODULE_NAME "${PROJECT_NAME}Kernel")

# Process template files.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/exports.yml.in" "${EXPORTS_YML}" @ONLY)

add_executable("${MODULE_NAME}"
  src/main.c
  src/bluetooth.c
  src/bt_event.c
  src/logfile.c
  src/umod_callback.c
)

target_compile_definitions("${MODULE_NAME}" PRIVATE ENABLE_VITASDK_BUILD_ASSERT)
common_compile_definitions("${MODULE_NAME}")
common_include_directories("${MODULE_NAME}")
common_compile_options("${MODULE_NAME}")
common_link_options("${MODULE_NAME}")

target_link_libraries("${MODULE_NAME}" PRIVATE
  gcc

  SceBtForDriver_stub
  SceDebugForDriver_stub
  SceIofilemgrForDriver_stub
  SceRtcForDriver_stub
  SceSysclibForDriver_stub
  SceSysmemForDriver_stub
  SceThreadmgrForDriver_stub
)

vita_create_self("${PROJECT_NAME}.skprx"
  "${MODULE_NAME}"
  CONFIG "${EXPORTS_YML}"
  UNSAFE
)

vita_create_stubs(stubs
  "${MODULE_NAME}"
  "${EXPORTS_YML}"
  KERNEL
)

add_library("${PROJECT_NAME}_stub" STATIC IMPORTED GLOBAL)

add_dependencies("${PROJECT_NAME}_stub" stubs)

set_target_properties("${PROJECT_NAME}_stub" PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/stubs/lib${PROJECT_NAME}_stub.a"
)
