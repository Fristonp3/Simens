cmake_minimum_required(VERSION 3.20)

project(gd32f470_industrial_acq C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Matches the original project layout where Firmware/ and Utilities/ are
# siblings of Template/.
set(GD32_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "GD32 SDK root directory")

set(GD32_CMSIS_DIR "${GD32_SDK_ROOT}/Firmware/CMSIS")
set(GD32_DEVICE_DIR "${GD32_CMSIS_DIR}/GD/GD32F4xx")
set(GD32_SPL_DIR "${GD32_SDK_ROOT}/Firmware/GD32F4xx_standard_peripheral")
set(GD32_UTIL_DIR "${GD32_SDK_ROOT}/Utilities")

set(LINKER_SCRIPT "${GD32_DEVICE_DIR}/Source/GCC/Ld/gd32f470xE_flash.ld")
set(STARTUP_FILE "${GD32_DEVICE_DIR}/Source/GCC/startup_gd32f450_470.S")
set(SYSTEM_FILE "${GD32_DEVICE_DIR}/Source/system_gd32f4xx.c")

set(MISSING_FILES "")
foreach(required_file IN LISTS LINKER_SCRIPT STARTUP_FILE SYSTEM_FILE)
    if(NOT EXISTS "${required_file}")
        list(APPEND MISSING_FILES "${required_file}")
    endif()
endforeach()

if(MISSING_FILES)
    message(FATAL_ERROR
        "Required GD32 SDK files are missing. Set GD32_SDK_ROOT to the folder containing Firmware/ and Utilities/.\n"
        "Missing:\n  ${MISSING_FILES}\n"
        "Example: cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake -DGD32_SDK_ROOT=/home/fristonp/Simens"
    )
endif()

file(GLOB GD32_SPL_SOURCES CONFIGURE_DEPENDS
    "${GD32_SPL_DIR}/Source/gd32f4xx_*.c"
)

file(GLOB APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/app/*.c"
)

file(GLOB BSP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/bsp/*.c"
)

file(GLOB SERVICE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/service/*.c"
)

file(GLOB COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.c"
)

file(GLOB BOOTLOADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/bootloader/*.c"
)

add_executable(${PROJECT_NAME}.elf
    main.c
    systick.c
    gd32f4xx_it.c
    board.c
    ${APP_SOURCES}
    ${BSP_SOURCES}
    ${SERVICE_SOURCES}
    ${COMMON_SOURCES}
    ${BOOTLOADER_SOURCES}
    "${SYSTEM_FILE}"
    "${STARTUP_FILE}"
    ${GD32_SPL_SOURCES}
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    USE_STDPERIPH_DRIVER
    GD32F470
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${GD32_CMSIS_DIR}"
    "${GD32_DEVICE_DIR}/Include"
    "${GD32_DEVICE_DIR}/Source/ARM"
    "${GD32_SPL_DIR}/Include"
    "${GD32_UTIL_DIR}"
)

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fmessage-length=0
    -fsigned-char
    -g
    -Og
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    --specs=nano.specs
    --specs=nosys.specs
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    VERBATIM
)

find_program(OPENOCD_EXECUTABLE openocd)
if(OPENOCD_EXECUTABLE)
    add_custom_target(flash
        COMMAND ${OPENOCD_EXECUTABLE}
            -f ${CMAKE_CURRENT_SOURCE_DIR}/openocd_stlink.cfg
            -c "program $<TARGET_FILE:${PROJECT_NAME}.elf> verify reset exit"
        DEPENDS ${PROJECT_NAME}.elf
        VERBATIM
    )
endif()
