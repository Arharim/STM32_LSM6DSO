cmake_minimum_required(VERSION 3.20)

project(STM32_LSM6DSO C ASM)

set(MCU_FLAGS -mcpu=cortex-m3 -mthumb)

add_compile_options(${MCU_FLAGS} -Wall -Wextra -g3 -O0)
add_compile_definitions(STM32F103xB)

add_link_options(${MCU_FLAGS}
    -T${CMAKE_SOURCE_DIR}/STM32F103XB_FLASH.ld
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

set(SOURCES
    src/main.c
    src/hal/clock.c
    src/hal/gpio.c
    src/hal/pwm.c
    src/hal/spi.c
    src/hal/timer.c
    src/hal/uart.c
    src/hal/watchdog.c
    src/app/fsm.c
    src/drivers/lsm6dso.c
    src/system_stm32f10x.c
    startup_stm32f103xb.s
)

add_executable(${PROJECT_NAME}.elf ${SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    include
    cmsis
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex  $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE}    $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Generating ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin"
)
