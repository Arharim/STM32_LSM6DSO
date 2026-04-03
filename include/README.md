# STM32_LSM6DSO — Firmware include directory

**Target MCU :** STM32F103 (Cortex-M3, medium density, 24 MHz system clock)
**Main sensor:** LSM6DSO 6-axis IMU (accelerometer + gyroscope) over SPI1

## Directory layout

```
include/
├── app/
│   └── fsm.h            — Finite state machine (INIT → STABILIZING → READ →
│                          PROCESS → OUTPUT → IDLE) driving the sensor cycle
├── drivers/
│   └── lsm6dso.h        — LSM6DSO driver: register map, init, read data,
│                          data-ready check (SPI Mode 3)
├── hal/
│   ├── clock.h          — RCC / PLL setup (HSE 8 MHz → SYSCLK 24 MHz),
│                          peripheral clock gating
│   ├── gpio.h           — GPIO pin init/deinit (SPI1 PA4-PA7, USART2 PA2-PA3,
│                          TIM1 PWM PA8-PA11)
│   ├── gpio_config.h    — Pin number constants, CRL/CRH mode nibbles, default
│                          register patterns
│   ├── pwm.h            — Servo PWM via TIM1 (4 ch, PA8-PA11) and TIM4 (2 ch,
│                          PB6-PB7), 50 Hz, 1–2 ms pulse range
│   ├── spi.h            — SPI1 master driver (Mode 3, software NSS), single-
│                          byte and burst read/write helpers
│   ├── timer.h          — SysTick (1 ms tick), TIM2 (100 Hz periodic), TIM3
│                          (1 kHz delay); delay_ms(), get_system_tick()
│   ├── uart.h           — USART2 115200 8N1, interrupt-driven TX ring buffer,
│                          optional debug printf
│   └── watchdog.h       — IWDG with configurable timeout, auto-reset on hang
├── config.h             — System-wide constants: clock, baud rate, timer
│                          frequencies, version, helper macros, compile-time
│                          assertions
├── stm32f10x.h          — CMSIS device header: register layouts, IRQ numbers,
│                          memory-mapped peripheral pointers
└── system_stm32f10x.h   — SystemInit() / SystemCoreClock declarations
```

## Layer dependencies (top → bottom)

```
app/fsm.h
    → drivers/lsm6dso.h
          → hal/spi.h  hal/timer.h
    → hal/uart.h  hal/pwm.h  hal/watchdog.h

All HAL modules depend on config.h and stm32f10x.h.
```

## Configuration

Edit `config.h` to change system clock, UART baud rate, timer frequencies,
watchdog timeout, or enable debug logging (`DEBUG_LOG_ENABLED`).
