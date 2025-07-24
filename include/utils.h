#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

// Type Definitions
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;      

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// Configuration Constants
#define SYSTEM_CLOCK_HZ     (24000000UL)
#define UART_BAUD_RATE      (115200UL)
#define TIMER_FREQ_HZ       (100UL)       // Main loop timer frequency
#define DELAY_TIMER_FREQ_HZ (1000UL)      // 1 kHz for delay timer

// Peripheral Configuration
#define UART_BRR    ((SYSTEM_CLOCK_HZ + (UART_BAUD_RATE/2U)) / UART_BAUD_RATE)

#define TIM2_PSC    ((SYSTEM_CLOCK_HZ / 100000UL) - 1UL)
#define TIM2_ARR    ((100000UL / TIMER_FREQ_HZ) - 1UL)

#define TIM3_PSC    ((SYSTEM_CLOCK_HZ / 1000000UL) - 1UL)
#define TIM3_ARR    (1000UL - 1UL)

#define SPI_UART_TIMEOUT      (1000U)
#define TIMER_CONFIG_TIMEOUT  (10000U)
// #define HSE_STARTUP_TIMEOUT   (0x5000U)
#define PLL_LOCK_TIMEOUT      (0x5000U)

// LSM6DSO Gyroscope Constants
#define LSM6DSO_WHO_AM_I    (0x0FU) // Chip ID register, ret 0x6C
#define LSM6DSO_CTRL1_XL    (0x10)  // Accelerometer control
#define LSM6DSO_CTRL2_G     (0x11U) // Gyro control register
#define LSM6DSO_CTRL3_C     (0x12U) // Ctrl register 3 (BDU, etc.)
#define LSM6DSO_OUTX_L_G    (0x22U) // Gyro X-axis data LSB (start of 6-byte burst)

#define LSM6DSO_ID_VAL      (0x6CU) // Expected chip ID
#define LSM6DSO_GYRO_CONFIG (0x4C)  // 208 Hz, 2000 dps
#define LSM6DSO_CTRL3_BDU   (0x40)  // Block Data Update enabled

// General purpose
// Bit Masks for 32-bit Registers
#define BIT(n) (1U << (n))

// Function Prototypes
void init_clocks(void);
void init_gpio(void);
void init_spi(void);
void init_uart(void);
void init_timer(void);
void delay_ms(u32 ms);
int spi_write(u8 reg, u8 data);
int spi_read(u8 reg, u8* data);
int spi_read_burst(u8 reg, u8* buffer, u8 len);
void uart_puts(const char *s);
u32 get_system_tick(void);


#endif // UTILS_H