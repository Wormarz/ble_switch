# BLE Switch: Zephyr application (pure C application logic)
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_switch)

# Application sources (C)
target_sources(app PRIVATE
  src/main.c
  src/ble_svc.c
  src/hw_glue.c
  src/button.c
  src/timer_glue.c
  src/app_logic.c
)
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

