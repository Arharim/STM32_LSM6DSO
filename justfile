build_dir := "build"
project   := "STM32_LSM6DSO"
elf       := build_dir + "/" + project + ".elf"
toolchain := "toolchain-arm.cmake"

# Configure and build project
build:
    cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE={{ toolchain }} -B {{ build_dir }}
    cmake --build {{ build_dir }}

# Flash via ST-Link
flash: build
    openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program {{ elf }} verify reset exit"

# Flash via J-Link
flash-jlink: build
    openocd -f interface/jlink.cfg -f target/stm32f1x.cfg \
        -c "program {{ elf }} verify reset exit"

# Remove build artifacts
clean:
    rm -rf {{ build_dir }}

# Clean and build
rebuild: (clean) (build)

# Clean, build, and flash via ST-Link
deploy: (clean) (flash)
