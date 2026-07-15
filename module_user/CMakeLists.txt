include("${CMAKE_SOURCE_DIR}/cmake/common.cmake" REQUIRED)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

# Set variables.
set(EXPORTS_YML "${CMAKE_CURRENT_BINARY_DIR}/cmake_gen/exports.yml")
set(MODULE_NAME "${PROJECT_NAME}User")
set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

# Process template files.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/exports.yml.in" "${EXPORTS_YML}" @ONLY)

add_executable("${MODULE_NAME}"
  src/main.c
  src/kmod_event.c
  src/logfile.c
  src/qm_state.c
  src/quickmenu.c
)

target_compile_definitions("${MODULE_NAME}" PRIVATE ENABLE_VITASDK_BUILD_ASSERT)
common_compile_definitions("${MODULE_NAME}")
common_include_directories("${MODULE_NAME}")
target_include_directories("${MODULE_NAME}" SYSTEM PRIVATE
  "${VENDOR_DIR}/quickmenureborn"
)
common_compile_options("${MODULE_NAME}")
common_link_options("${MODULE_NAME}")

target_link_directories("${MODULE_NAME}" PRIVATE
  "${VENDOR_DIR}/quickmenureborn/vitasdk_stubs"
)

target_link_libraries("${MODULE_NAME}" PRIVATE
  gcc

  # SceLibc_stub: not available, use sceClibMemset instead of `= {0}`.
  SceIofilemgr_stub
  SceKernelThreadMgr_stub
  SceLibKernel_stub
  SceRtc_stub

  taihen_stub

  QuickMenuReborn_stub_weak

  "${PROJECT_NAME}_stub"
)

vita_create_self("${PROJECT_NAME}.suprx"
  "${MODULE_NAME}"
  CONFIG "${EXPORTS_YML}"
  UNSAFE
)
