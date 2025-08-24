# STM32F103 + LSM6DSO Gyroscope (CMSIS, PlatformIO)

Bare‑metal STM32F103 (Blue Pill) firmware using CMSIS and direct register access to interface with an ST LSM6DSO IMU over SPI. The firmware initializes clocks, GPIO, SPI1, USART1, and timers, configures the LSM6DSO gyroscope, and periodically reads gyro XYZ data and streams formatted values over UART.

- Board: genericSTM32F103C8T6 (Blue Pill)
- Framework: CMSIS/SPL (no HAL)
- Build system: PlatformIO
- License: Apache 2.0

## Features
- Direct register CMSIS implementation (no HAL/LL)
- SPI1 Mode 3 (CPOL=1, CPHA=1) communication with LSM6DSO
- Initializes gyro: ODR=208 Hz, FS=2000 dps; enables BDU + auto‑increment
- Finite state machine for non‑blocking startup, read, process, output
- TIM2 generates 100 Hz “tick” for sampling; TIM3 used for delays
- SysTick 1 ms system tick
- UART1 115200 8N1 output of "X: Y: Z:" lines

## Hardware
- MCU: STM32F103C8T6 (Blue Pill)
- IMU: ST LSM6DSO(STEVAL-MKI196V1) (tested with WHO_AM_I = 0x6C)

Recommended wiring (SPI1 and UART1):
- PA4  -> LSM6DSO CS (active low)
- PA5  -> LSM6DSO SCK
- PA6  -> LSM6DSO MISO
- PA7  -> LSM6DSO MOSI
- 3.3V -> LSM6DSO VDD and VDDIO
- GND  -> LSM6DSO GND
- PA9 (USART1 TX)  -> USB‑UART RX (host)
- PA10 (USART1 RX) -> USB‑UART TX (host) [optional]

Notes
- SPI mode is 3; ensure the IMU board supports 3.3V logic.
- Default SPI clock ≈ APB2/32; with SYSCLK=24 MHz, SPI ≈ 750 kHz.

## Build and Flash
Requirements
- VS Code with PlatformIO extension, or PlatformIO CLI
- ST‑Link/V2 (recommended) or another supported uploader

PlatformIO environment
- Defined in platformio.ini as `[env:genericSTM32F103C8]`
- Framework: cmsis
- Build flags: -DSTM32F103xB

Using PlatformIO (CLI):
- Build: `pio run`
- Upload: `pio run -t upload`
- Monitor: `pio device monitor -b 115200`

Using VS Code (PIO extension):
- Project Tasks → Build / Upload / Monitor

If upload fails, set upload protocol/port in platformio.ini, for example:
- `upload_protocol = stlink`

## Runtime Behavior
On reset the firmware:
- Configures system clock to 24 MHz (HSE * 3), SysTick at 1 ms
- Initializes GPIO, SPI1, USART1 (115200 8N1), TIM2 (100 Hz), TIM3 (1 kHz)
- Programs LSM6DSO CTRL2_G=0x60 (208 Hz, 2000 dps), CTRL3_C=BDU|IF_INC
- Verifies WHO_AM_I (0x6C) with limited retries and stabilization delay
- Prints a one‑time line like: `Timers OK: TIM2=100Hz TIM3=1000Hz`
- Periodically (100 Hz) prints lines: `X: data Y: data Z: data `

Error handling
- On SPI/ID errors the system prints an error message and halts

## Configuration
Key tunables in include/utils.h:
- SYSTEM_CLOCK_HZ: 24000000
- UART_BAUD_RATE: 115200 (affects UART_BRR)
- TIMER_FREQ_HZ: 100 (TIM2 sample rate)
- DELAY_TIMER_FREQ_HZ: 1000 (TIM3)
- SPI_UART_TIMEOUT, TIMER_CONFIG_TIMEOUT, PLL_LOCK_TIMEOUT
- LSM6DSO register addresses and constants; expected ID 0x6C

Application logic (src/MAIN.c):
- State machine with states INIT → STABILIZING → READ → PROCESS → OUTPUT → IDLE
- Non‑blocking stabilization window and WHO_AM_I retries
- Burst read of 6 bytes from OUTX/Y/Z_L_G for gyro data
- UART output via interrupt‑driven circular buffer

## Project Structure
- platformio.ini — PlatformIO environment (ststm32, genericSTM32F103C8, cmsis)
- src/MAIN.c — main application, drivers (SPI/UART/Timers), FSM
- include/utils.h — types, constants, and function prototypes
- include/stm32f10x.h, system_stm32f10x.h — CMSIS device/system headers
- LICENSE — Apache 2.0

## Troubleshooting
- No output: check 115200 baud on the correct serial port; ensure USART1 TX is connected
- Wrong/constant values: confirm SPI mode 3 wiring and CS line (PA4), and that IF_INC is enabled
- Upload errors: configure `upload_protocol = stlink` and connect ST‑Link to SWD pins

## License
This project is licensed under the Apache License 2.0. See LICENSE for details.
