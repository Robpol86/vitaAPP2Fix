# Shared CMake helpers for the project.

# Set common compile definitions.
function(common_compile_definitions target)
  target_compile_definitions("${target}" PRIVATE
    MODULE_NAME="${MODULE_NAME}"
    PROJECT_NAME="${PROJECT_NAME}"
  )
endfunction()

# Set common include directories.
function(common_include_directories target)
  target_include_directories("${target}" PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/cmake_gen"
  )
endfunction()

# Set common compile options.
function(common_compile_options target)
  target_compile_options("${target}" PRIVATE
    -Wall
    -Werror=format-security
    -Werror=strict-prototypes
    -Werror=unused-function
    -Wformat=2
    -ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/src/=
  )
endfunction()

# Set common link options.
function(common_link_options target)
  target_link_options("${target}" PRIVATE
    -Wl,-e,module_start
    -Wl,-q
    -nostdlib
  )
endfunction()
