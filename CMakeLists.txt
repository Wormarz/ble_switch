# BLE Switch: Zephyr application (pure C application logic)
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_switch)

# Application sources (C)
target_sources(app PRIVATE
  src/main.c
  src/ble/ble_service.c
  src/motor/motor_driver.c
  src/motor/motor_fsm.c
  src/UI/button.c
  src/UI/led.c
  src/power/battery.c
  src/power/sleep.c
  src/storage/nvs_storage.c
)
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ble
  ${CMAKE_CURRENT_SOURCE_DIR}/src/motor
  ${CMAKE_CURRENT_SOURCE_DIR}/src/power
  ${CMAKE_CURRENT_SOURCE_DIR}/src/UI
  ${CMAKE_CURRENT_SOURCE_DIR}/src/storage
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

