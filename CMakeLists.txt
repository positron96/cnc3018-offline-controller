set(CMAKE_TOOLCHAIN_FILE cmake/ArduinoToolchain.cmake)
project(cnc_3018 C CXX)
cmake_minimum_required(VERSION 3.22)

SET(TYPE TOOLCHAIN)


if (TYPE MATCHES "TOOLCHAIN")

    set(BOARD_ID GENERIC_F103CBTX)
#    SET(ARDUINO_SDK_PATH $ENV{HOME}/.platformio/packages/framework-arduinoststm32 )
    generate_arduino_firmware(exe_name
            BOARD GENERIC_F103CBTX
            LIBS U8g2 SD "Embedded Template Library"
            SRCS src/main.cpp)
else ()
    set(CMAKE_CONFIGURATION_TYPES "controller" CACHE STRING "Build Types reflect PlatformIO Environments" FORCE)
    set(CMAKE_CONFIGURATION_TYPES "debug" CACHE STRING "Build Types reflect PlatformIO Environments" FORCE)


    SET(CMAKE_C_COMPILER "$ENV{HOME}/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-gcc")
    SET(CMAKE_CXX_COMPILER "$ENV{HOME}/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-g++")

    SET(COMMON_FLAGS "-Os -Wall -ffunction-sections -fdata-sections -flto")
    SET(CMAKE_CXX_FLAGS "-fno-exceptions -fno-threadsafe-statics  -std=gnu++11  ${COMMON_FLAGS}") #-fpermissive")
    SET(CMAKE_C_FLAGS "-std=gnu11 -fno-fat-lto-objects ${COMMON_FLAGS}")

    SET(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 11)

    add_executable(Production src/main.cpp)
    add_executable(debug src/main.cpp)

    set(ARDUINO_DEFAULT_BOARD uno) # Default Board ID
    set(ARDUINO_DEFAULT_PORT /dev/ttyACM0) # Default Port

    FILE(GLOB_RECURSE SRC_LIST "${CMAKE_CURRENT_LIST_DIR}/src/*.*" "${CMAKE_CURRENT_LIST_DIR}/lib/*.*")


    if (CMAKE_BUILD_TYPE MATCHES "controller")
        # libs
        include_directories("${CMAKE_CURRENT_LIST_DIR}/.pio/libdeps/controller/U8g2/src")
        include_directories("${CMAKE_CURRENT_LIST_DIR}/.pio/libdeps/controller/Embedded Template Library/include")
        include_directories("${CMAKE_CURRENT_LIST_DIR}/.pio/libdeps/controller/Embedded Template Library")
        include_directories("${CMAKE_CURRENT_LIST_DIR}/.pio/libdeps/controller/SD/src")

        add_definitions(-D'LOG_DEBUG')
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
        #    old
        #    add_definitions(-D'PLATFORMIO=40300')
        #    add_definitions(-D'ARDUINO_AVR_UNO')
        #    add_definitions(-D'F_CPU=16000000L')
        #    add_definitions(-D'ARDUINO_ARCH_AVR')
        #    add_definitions(-D'ARDUINO=10808')
        #    add_definitions(-D'__AVR_ATmega328P__')

        include_directories("${CMAKE_CURRENT_LIST_DIR}/include")
        include_directories("${CMAKE_CURRENT_LIST_DIR}/lib")
        include_directories("${CMAKE_CURRENT_LIST_DIR}/src")
        include_directories("/home/lima/cnc/cnc3018-offline-controller/lib/printfloat")

        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/cores/arduino")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/variants/STM32F2xx")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/SPI/src")


        include_directories("/home/lima/.platformio/packages/framework-arduinoststm32/libraries/Wire/src")

        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/SPI/src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/cores/arduino/avr")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/cores/arduino/stm32")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/cores/arduino/stm32/LL")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/cores/arduino/stm32/usb")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/cores/arduino/stm32/OpenAMP")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/cores/arduino/stm32/usb/hid")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/cores/arduino/stm32/usb/cdc")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Drivers/STM32F1xx_HAL_Driver/Inc")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Drivers/STM32F1xx_HAL_Driver/Src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/STM32F1xx")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Middlewares/ST/STM32_USB_Device_Library/Core/Inc")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Middlewares/ST/STM32_USB_Device_Library/Core/Src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Middlewares/OpenAMP")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Middlewares/OpenAMP/open-amp/lib/include")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Middlewares/OpenAMP/libmetal/lib/include")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Middlewares/OpenAMP/virtual_driver")
        include_directories("$ENV{HOME}/.platformio/packages/framework-cmsis/CMSIS/Core/Include")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Drivers/CMSIS/Device/ST/STM32F1xx/Include")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/system/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc")
        include_directories("$ENV{HOME}/.platformio/packages/framework-cmsis/CMSIS/DSP/Include")
        include_directories("$ENV{HOME}/.platformio/packages/framework-cmsis/CMSIS/DSP/PrivateInclude")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/cores/arduino")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/variants/STM32F1xx/F103C8T_F103CB(T-U)")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/CMSIS_DSP/src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/EEPROM/src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/IWatchdog/src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/Keyboard/src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/Mouse/src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/Servo/src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/SoftwareSerial/src")
        include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoststm32/libraries/SrcWrapper/src")

    endif ()
endif ()







