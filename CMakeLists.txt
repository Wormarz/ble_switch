# BLE Switch: Zephyr application with Rust static library
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_switch)

# Build Rust static library (no_std, thumbv7em-none-eabihf for nRF52840)
set(RUST_TARGET "thumbv7em-none-eabihf")
set(RUST_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rust)
set(RUST_LIB_PATH ${RUST_LIB_DIR}/target/${RUST_TARGET}/release/libapp_rust.a)
add_custom_command(
  OUTPUT ${RUST_LIB_PATH}
  COMMAND cargo build --release --target ${RUST_TARGET}
  WORKING_DIRECTORY ${RUST_LIB_DIR}
  COMMENT "Building Rust static library"
)
add_custom_target(app_rust_target DEPENDS ${RUST_LIB_PATH})
add_library(app_rust STATIC IMPORTED GLOBAL)
set_target_properties(app_rust PROPERTIES
  IMPORTED_LOCATION ${RUST_LIB_PATH}
  IMPORTED_LINK_INTERFACE_LANGUAGES "C"
)
add_dependencies(app_rust app_rust_target)

# Application sources (C)
target_sources(app PRIVATE
  src/main.c
  src/ble_svc.c
  src/hw_glue.c
  src/button.c
  src/timer_glue.c
)
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(app PRIVATE app_rust)
