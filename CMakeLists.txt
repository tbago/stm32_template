cmake_minimum_required(VERSION 3.20)

project(template C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(STM32_DEVICE_DEFINE "STM32F10X_MD" CACHE STRING "STM32 device define from stm32f10x.h")
set(STM32_FLASH_SIZE "64K" CACHE STRING "Flash size used by the linker script")
set(STM32_RAM_SIZE "20K" CACHE STRING "RAM size used by the linker script")
set(STM32_HSE_VALUE "8000000" CACHE STRING "External crystal frequency in Hz")
set(STM32_FLASH_ADDRESS "0x08000000" CACHE STRING "Flash start address for binary programming")
set(STM32_PROGRAMMER_PORT "SWD" CACHE STRING "STM32_Programmer_CLI connection port")
option(STM32_USE_FULL_ASSERT "Enable USE_FULL_ASSERT in the StdPeriph library" OFF)

set(STM32_CMSIS_CORE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/startup/core_support")
set(STM32_CMSIS_DEVICE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/startup/device_support")
set(STM32_DRIVER_DIR
    "${CMAKE_CURRENT_LIST_DIR}/library")
set(STM32_STARTUP_FILE
    "${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32f10x_md.s")

foreach(path IN ITEMS "${STM32_CMSIS_CORE_DIR}" "${STM32_CMSIS_DEVICE_DIR}" "${STM32_DRIVER_DIR}" "${STM32_STARTUP_FILE}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "Required STM32 system path not found: ${path}")
    endif()
endforeach()

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/stm32f10x_flash.ld.in"
    "${CMAKE_CURRENT_BINARY_DIR}/stm32f10x_flash.ld"
    @ONLY)

find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(CMAKE_SIZE arm-none-eabi-size REQUIRED)
find_program(STM32_PROGRAMMER_CLI STM32_Programmer_CLI)

file(GLOB STM32_DRIVER_SOURCES CONFIGURE_DEPENDS
    "${STM32_DRIVER_DIR}/src/*.c")

set(STM32_COMMON_COMPILE_OPTIONS
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -Wshadow
    -Wundef)

add_library(stm32_common_options INTERFACE)
target_compile_options(stm32_common_options INTERFACE ${STM32_COMMON_COMPILE_OPTIONS})
target_compile_definitions(stm32_common_options INTERFACE
    USE_STDPERIPH_DRIVER
    ${STM32_DEVICE_DEFINE}
    HSE_VALUE=${STM32_HSE_VALUE}U)

if(STM32_USE_FULL_ASSERT)
    target_compile_definitions(stm32_common_options INTERFACE USE_FULL_ASSERT)
endif()

target_include_directories(stm32_common_options INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/library"
    "${STM32_CMSIS_CORE_DIR}"
    "${STM32_CMSIS_DEVICE_DIR}"
    "${STM32_DRIVER_DIR}/inc")

add_library(stm32_stdperiph STATIC
    ${STM32_DRIVER_SOURCES}
    "${STM32_CMSIS_DEVICE_DIR}/system_stm32f10x.c")
target_link_libraries(stm32_stdperiph PUBLIC stm32_common_options)

add_executable(${PROJECT_NAME}
    app/main.c
    app/stm32f10x_it.c
    "${STM32_STARTUP_FILE}")
target_link_libraries(${PROJECT_NAME} PRIVATE stm32_stdperiph stm32_common_options)
target_link_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${PROJECT_NAME}.map
    -T${CMAKE_CURRENT_BINARY_DIR}/stm32f10x_flash.ld)

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
    BYPRODUCTS
        ${PROJECT_NAME}.hex
        ${PROJECT_NAME}.bin
    VERBATIM)

if(STM32_PROGRAMMER_CLI)
    add_custom_target(flash
        COMMAND ${STM32_PROGRAMMER_CLI} -c port=${STM32_PROGRAMMER_PORT} -w $<TARGET_FILE:${PROJECT_NAME}> -v -rst
        DEPENDS ${PROJECT_NAME}
        USES_TERMINAL
        VERBATIM)

    add_custom_target(flash-bin
        COMMAND ${STM32_PROGRAMMER_CLI} -c port=${STM32_PROGRAMMER_PORT} -w ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin ${STM32_FLASH_ADDRESS} -v -rst
        DEPENDS ${PROJECT_NAME}
        USES_TERMINAL
        VERBATIM)
else()
    message(STATUS "STM32_Programmer_CLI not found, flash targets are disabled.")
endif()
