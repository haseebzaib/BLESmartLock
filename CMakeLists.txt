cmake_minimum_required(VERSION 3.20.0)

set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/dts)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(BLESmartLock)

target_sources(app PRIVATE 
src/main.c
src/BLE/BLEapp.c
src/crypto/cryptoapp.c
src/motorcontrol/mcapp.c
src/System/bib.c
src/System/sys_flash.c
)


# Include application events and configuration headers
target_include_directories(app PRIVATE
  src
  src/BLE
  src/crypto
  src/motorcontrol
  src/System
)