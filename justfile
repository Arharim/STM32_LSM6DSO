build_dir := "build"
project   := "STM32_LSM6DSO"
elf       := build_dir + "/" + project + ".elf"
toolchain := "toolchain-arm.cmake"

# Configure and build project (default: debug)
build type="debug":
    cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE={{ toolchain }} -DCMAKE_BUILD_TYPE={{ if type == "release" { "Release" } else { "Debug" } }} -B {{ build_dir }}
    cmake --build {{ build_dir }}

# Flash via ST-Link
flash type="debug": (build type)
    openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program {{ elf }} verify reset exit"

# Flash via J-Link
flash-jlink type="debug": (build type)
    openocd -f interface/jlink.cfg -f target/stm32f1x.cfg \
        -c "program {{ elf }} verify reset exit"

# Remove build artifacts
clean:
    rm -rf {{ build_dir }}

# Clean and build
rebuild type="debug": (clean) (build type)

# Clean, build, and flash via ST-Link
deploy type="debug": (clean) (flash type)
