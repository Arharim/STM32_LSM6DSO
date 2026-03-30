# STM32F103 + LSM6DSO Gyroscope (CMSIS, CMake/Ninja)

Bare-metal STM32F103 (Blue Pill) firmware using CMSIS and direct register access to interface with an ST LSM6DSO IMU over SPI. The firmware initializes clocks, GPIO, SPI1, USART2, and timers, configures the LSM6DSO gyroscope, and periodically reads gyro XYZ data and streams formatted values over UART.

- Board: genericSTM32F103C8T6 (Blue Pill)
- Framework: CMSIS (no HAL/LL)
- Build system: CMake + Ninja + just
- License: Apache 2.0

## Features
- Direct register CMSIS implementation (no HAL/LL)
- Modular architecture with separate HAL, drivers, and application layers
- SPI1 Mode 3 (CPOL=1, CPHA=1) communication with LSM6DSO
- Initializes gyro: ODR=208 Hz, FS=2000 dps; enables BDU + auto-increment
- Finite state machine for non-blocking startup, read, process, output
- TIM2 generates 100 Hz "tick" for sampling; TIM3 used for delays
- SysTick 1 ms system tick
- UART2 115200 8N1 output of "X: Y: Z:" lines
- PWM generation for 6 servo motors (50 Hz, 1-2 ms pulse)
- Error handling with HSE/PLL fallback to HSI

## Hardware
- MCU: STM32F103C8T6 (Blue Pill)
- IMU: ST LSM6DSO (STEVAL-MKI196V1) (tested with WHO_AM_I = 0x6C)

Recommended wiring (SPI1, UART2, PWM):
- PA4  -> LSM6DSO CS (active low)
- PA5  -> LSM6DSO SCK
- PA6  -> LSM6DSO MISO
- PA7  -> LSM6DSO MOSI
- 3.3V -> LSM6DSO VDD and VDDIO
- GND  -> LSM6DSO GND
- PA2 (USART2 TX)  -> USB-UART RX (host)
- PA3 (USART2 RX) -> USB-UART TX (host) [optional]
- PA8-PA11      -> Servo 1-4 signal (TIM1 PWM)
- PB6-PB7        -> Servo 5-6 signal (TIM4 PWM)

Notes:
- SPI mode is 3; ensure the IMU board supports 3.3V logic.
- Default SPI clock ≈ APB2/16; with SYSCLK=24 MHz, SPI ≈ 1.5 MHz.
- PWM servos: 50 Hz, pulse 1-2 ms for 0-180°.

## Build and Flash
Requirements:
- arm-none-eabi-gcc (ARM embedded toolchain)
- CMake 3.20+
- Ninja 1.11+
- just (command runner)
- OpenOCD (for flashing)

Quick commands (run from project root):
- `just build` — configure and compile
- `just flash` — build and flash via ST-Link
- `just flash-jlink` — build and flash via J-Link
- `just clean` — remove build artifacts
- `just rebuild` — clean and build
- `just deploy` — clean, build, and flash via ST-Link

Run `just --list` to see all available recipes.

## Project Structure
```
├── CMakeLists.txt          # Build configuration
├── toolchain-arm.cmake     # ARM GCC cross-compilation toolchain
├── justfile                # Short build/flash/clean commands
├── startup_stm32f103xb.s   # Vector table and startup code
├── STM32F103XB_FLASH.ld    # Linker script (64K Flash, 20K RAM)
├── cmsis/                  # CMSIS Core headers
│   ├── core_cm3.h
│   ├── cmsis_compiler.h
│   ├── cmsis_gcc.h
│   ├── cmsis_version.h
│   └── mpu_armv7.h
├── include/
│   ├── config.h            # System configuration constants
│   ├── stm32f10x.h         # Device peripheral definitions
│   ├── system_stm32f10x.h  # System init header
│   ├── hal/
│   │   ├── clock.h         # RCC, PLL configuration
│   │   ├── gpio.h          # GPIO initialization
│   │   ├── gpio_config.h   # GPIO pin definitions
│   │   ├── pwm.h           # PWM driver for servos
│   │   ├── spi.h           # SPI driver
│   │   ├── timer.h         # SysTick, TIM2, TIM3
│   │   └── uart.h          # UART with TX buffer
│   ├── drivers/
│   │   └── lsm6dso.h       # LSM6DSO IMU driver
│   └── app/
│       └── fsm.h           # Finite state machine
└── src/
    ├── main.c              # Entry point
    ├── system_stm32f10x.c  # SystemInit implementation
    ├── hal/
    │   ├── clock.c
    │   ├── gpio.c
    │   ├── pwm.c
    │   ├── spi.c
    │   ├── timer.c
    │   └── uart.c
    ├── drivers/
    │   └── lsm6dso.c       # LSM6DSO implementation
    └── app/
        └── fsm.c           # FSM implementation
```

## API Reference

### HAL Layer

#### Clock (`hal/clock.h`)
```c
clock_status_t clock_init(void);  // Initialize system clock, returns CLOCK_OK or error
```
- On HSE/PLL error, falls back to HSI and returns error code
 - Enables clocks for GPIOA, SPI1, USART2, TIM1, TIM2, TIM3, TIM4

#### GPIO (`hal/gpio.h`)
```c
void gpio_init(void);    // Configure all GPIO pins
void gpio_deinit(void);  // Disable GPIO clocks
```

#### SPI (`hal/spi.h`)
```c
void spi_init(void);
void spi_deinit(void);
void spi_cs_high(void);
void spi_cs_low(void);
spi_status_t spi_write_byte(uint8_t reg, uint8_t data);
spi_status_t spi_read_byte(uint8_t reg, uint8_t *data);
spi_status_t spi_read_burst(uint8_t reg, uint8_t *buffer, uint8_t len);
```
- Mode 3 (CPOL=1, CPHA=1), Master, Software NSS
- Timeout: 1000ms

#### UART (`hal/uart.h`)
```c
void uart_init(void);
void uart_deinit(void);
void uart_puts(const char *s);
uart_status_t uart_puts_safe(const char *s);
bool uart_is_enabled(void);
void uart_clear_errors(void);
```
- 115200 baud, 8N1
- TX buffer: 256 bytes, interrupt-driven

#### Timer (`hal/timer.h`)
```c
void timer_init(void);
void timer_deinit(void);
void delay_ms(uint32_t ms);
uint32_t get_system_tick(void);
bool timer_is_running(uint32_t timer_base);
```
- SysTick: 1ms system tick
- TIM2: 100Hz sample timer
- TIM3: 1kHz delay timer

#### PWM (`hal/pwm.h`)
```c
void pwm_init(void);
void pwm_deinit(void);
pwm_status_t pwm_set_pulse_us(pwm_channel_t channel, uint16_t pulse_us);
pwm_status_t pwm_set_angle(pwm_channel_t channel, uint8_t angle);
pwm_status_t pwm_set_pulse_all(const uint16_t *pulse_us);
bool pwm_is_enabled(void);
```
- Frequency: 50 Hz (20 ms period)
- Pulse width: 1000-2000 µs (1-2 ms) for 0-180°
- Channels: PA8-PA11 (TIM1), PB6-PB7 (TIM4)

### Driver Layer

#### LSM6DSO (`drivers/lsm6dso.h`)
```c
lsm6dso_status_t lsm6dso_init(void);
lsm6dso_status_t lsm6dso_read_id(uint8_t *id);
lsm6dso_status_t lsm6dso_read_data(lsm6dso_data_t *data);
bool lsm6dso_data_ready(void);
```
- Gyro: 208Hz, ±2000dps
- Accel: 208Hz, ±2g
- BDU enabled, auto-increment enabled

### Application Layer

#### FSM (`app/fsm.h`)
```c
void fsm_init(void);
void fsm_run(void);
void fsm_set_state(fsm_state_t new_state);
fsm_state_t fsm_get_state(void);
```

States:
- `FSM_STATE_INIT` → Initialize hardware
- `FSM_STATE_STABILIZING` → Wait 100ms for sensor
- `FSM_STATE_READ` → Read sensor data
- `FSM_STATE_PROCESS` → Format data
- `FSM_STATE_OUTPUT` → Send via UART
- `FSM_STATE_IDLE` → Wait for next tick (WFI)
- `FSM_STATE_ERROR` → Halt on error

## Runtime Behavior
On reset the firmware:
1. Configures system clock to 24 MHz (HSE × 3), SysTick at 1 ms
2. Initializes GPIO, SPI1, USART2 (115200 8N1), TIM1/TIM4 (PWM 50 Hz), TIM2 (100 Hz), TIM3 (1 kHz)
3. Programs LSM6DSO: CTRL2_G=0x5C (208 Hz, 2000 dps), CTRL3_C=BDU|IF_INC
4. Verifies WHO_AM_I (0x6C) with 3 retries and 100ms stabilization
5. Periodically (100 Hz) prints: `X: data Y: data Z: data `

Error handling:
- HSE/PLL failure: Falls back to HSI (8 MHz), continues operation
- SPI timeout: Returns error code, FSM enters ERROR state
- Invalid ID: FSM retries, then enters ERROR state

## Configuration
Key tunables in `include/config.h`:
- `SYSTEM_CLOCK_HZ`: 24000000
- `UART_BAUD_RATE`: 115200
- `TIMER_FREQ_HZ`: 100 (TIM2 sample rate)
- `DELAY_TIMER_FREQ_HZ`: 1000 (TIM3)

Pin definitions in `include/hal/gpio_config.h`:
- `SPI_CS_PIN`, `UART_TX_PIN`, etc.

## Troubleshooting
- No output: Check 115200 baud, correct serial port, USART2 TX (PA2) connected
- Wrong/constant values: Confirm SPI mode 3 wiring and CS line (PA4)
- Upload errors: Verify ST-Link/J-Link connection and OpenOCD installation
- HSE timeout: Check external crystal, firmware will fall back to HSI
- Compiler not found: Ensure arm-none-eabi-gcc is in PATH

## License
This project is licensed under the Apache License 2.0. See LICENSE for details.
