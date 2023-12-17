set(CMAKE_SYSTEM_NAME Arduino)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(tools $ENV{HOME}/.platformio/packages/toolchain-gccarmnoneeabi)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
#set(CMAKE_TOOLCHAIN_FILE  cmake/ArduinoToolchain.cmake)


project(cnc_3018 C CXX)
cmake_minimum_required(VERSION 3.00)
SET(COMMON_FLAGS "-Os -Wall -ffunction-sections -fdata-sections -flto")
SET(CMAKE_CXX_FLAGS "-fno-exceptions -fno-threadsafe-statics  -std=gnu++11  ${COMMON_FLAGS}") #-fpermissive")
SET(CMAKE_C_FLAGS "-std=gnu11 -fno-fat-lto-objects ${COMMON_FLAGS}")

SET(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_C_COMPILER "${tools}/bin/arm-none-eabi-gcc")
SET(CMAKE_CXX_COMPILER "${tools}/bin/arm-none-eabi-g++")

#add_definitions(-D'LOG_DEBUG')
#from definitions in json
add_definitions(-D'PLATFORMIO=60111')
add_definitions(-D'STM32F103xB')
add_definitions(-D'STM32F1')
add_definitions(-D'USBCON')
add_definitions(-D'USBD_USE_CDC')
add_definitions(-D'ENABLE_HWSERIAL1')
add_definitions(-D'U8G2_16BIT')
add_definitions(-D'STM32F1xx')
add_definitions(-D'ARDUINO=10808')
add_definitions(-D'ARDUINO_ARCH_STM32')
add_definitions(-D'ARDUINO_GENERIC_F103CBTX')
add_definitions(-D'BOARD_NAME=GENERIC_F103CBTX')
add_definitions(-D'HAL_UART_MODULE_ENABLED')
add_definitions(-D'USE_FULL_LL_DRIVER')
add_definitions(-D'VARIANT_H="variant_generic.h"')
add_definitions(-D'HAL_PCD_MODULE_ENABLED')


include_directories("${CMAKE_CURRENT_LIST_DIR}/.pio/libdeps/controller/U8g2/src")
include_directories("${CMAKE_CURRENT_LIST_DIR}/.pio/libdeps/controller/Embedded Template Library/include")
include_directories("${CMAKE_CURRENT_LIST_DIR}/.pio/libdeps/controller/Embedded Template Library")
include_directories("${CMAKE_CURRENT_LIST_DIR}/.pio/libdeps/controller/SD/src")

SET(PLATFORMIO_HOME $ENV{HOME}/.platformio/packages)
SET(FRAMEWORK ${PLATFORMIO_HOME}/framework-arduinoststm32)
SET(FRAMEWORK_CMSIS ${PLATFORMIO_HOME}/framework-cmsis/CMSIS)

add_library(printfloat STATIC
        lib/printfloat/printfloat.cpp
        )
target_include_directories(printfloat
        PUBLIC lib/printfloat)

FILE(GLOB_RECURSE SRC_LIST "src/*.cpp")
add_executable(main ${SRC_LIST})

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${FRAMEWORK}/system/ldscript.ld")

target_include_directories(main
        PUBLIC src
        src/ui
        src/gcode
        src/devices
        lib/printfloat/
        )

link_directories(BEFORE ${PLATFORMIO_HOME}/toolchain-gccarmnoneeabi/lib)
link_directories(${PLATFORMIO_HOME}/toolchain-gccarmnoneeabi/lib/gcc/arm-none-eabi/9.2.1)

target_link_libraries(main printfloat)


set(CMAKE_CONFIGURATION_TYPES "controller" CACHE STRING "Build Types reflect PlatformIO Environments" FORCE)


include_directories("${FRAMEWORK}/cores/arduino")
include_directories("${FRAMEWORK}/cores/arduino/stm32")
include_directories("${FRAMEWORK}/cores/arduino/stm32/LL")
include_directories("${FRAMEWORK}/cores/arduino/stm32/usb")
include_directories("${FRAMEWORK}/variants/STM32F1xx/F103C8T_F103CB(T-U)")
include_directories("${FRAMEWORK_CMSIS}/CMSIS/Core/Include")
include_directories("${FRAMEWORK_CMSIS}/Core/Include")
include_directories("${FRAMEWORK_CMSIS}/DSP/Include")
include_directories("${FRAMEWORK_CMSIS}/DSP/PrivateInclude")

include_directories("${FRAMEWORK}/system/STM32F1xx")
include_directories("${FRAMEWORK}/system/Drivers/CMSIS/Device/ST/STM32F1xx/Include")
include_directories("${FRAMEWORK}/system/Drivers/CMSIS/Device/ST/STM32F1xx/Include")
include_directories("${FRAMEWORK}/system/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc")
include_directories("${FRAMEWORK}/system/Drivers/STM32F1xx_HAL_Driver/Inc")
include_directories("${FRAMEWORK}/system/Drivers/STM32F1xx_HAL_Driver/Src")
include_directories("${FRAMEWORK}/system/Middlewares/ST/STM32_USB_Device_Library/Core/Inc")
include_directories("${FRAMEWORK}/system/Middlewares/ST/STM32_USB_Device_Library/Core/Src")

