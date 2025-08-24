#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

// Type Definitions
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// Configuration Constants
#define SYSTEM_CLOCK_HZ     (24000000UL)
#define UART_BAUD_RATE      (115200UL)
#define TIMER_FREQ_HZ       (100UL)  // Main loop timer frequency
#define DELAY_TIMER_FREQ_HZ (1000UL) // 1 kHz for delay timer

// Peripheral Configuration
#define UART_BRR ((SYSTEM_CLOCK_HZ + (UART_BAUD_RATE / 2U)) / UART_BAUD_RATE)

#define TIM2_PSC ((SYSTEM_CLOCK_HZ / 100000UL) - 1UL)
#define TIM2_ARR ((100000UL / TIMER_FREQ_HZ) - 1UL)
#define TIM3_PSC ((SYSTEM_CLOCK_HZ / 1000000UL) - 1UL)
#define TIM3_ARR (1000UL - 1UL)

#define SPI_UART_TIMEOUT     (1000U)
#define TIMER_CONFIG_TIMEOUT (10000U)
#define PLL_LOCK_TIMEOUT     (0x5000U)
// #define HSE_STARTUP_TIMEOUT   (0x5000U)

// LSM6DSO Gyroscope Constants
#define LSM6DSO_WHO_AM_I   (0x0FU)
#define LSM6DSO_CTRL1_XL   (0x10U)
#define LSM6DSO_CTRL2_G    (0x11U)
#define LSM6DSO_CTRL3_C    (0x12U)
#define LSM6DSO_CTRL4_C    (0x13U)
#define LSM6DSO_CTRL5_C    (0x14U)
#define LSM6DSO_CTRL6_C    (0x15U)
#define LSM6DSO_CTRL7_G    (0x16U)
#define LSM6DSO_CTRL8_XL   (0x17U)
#define LSM6DSO_CTRL9_XL   (0x18U)
#define LSM6DSO_CTRL10_C   (0x19U)
#define LSM6DSO_STATUS_REG (0x1EU)
#define LSM6DSO_OUT_TEMP_L (0x20U)
#define LSM6DSO_OUT_TEMP_H (0x21U)
#define LSM6DSO_OUTX_L_G   (0x22U)
#define LSM6DSO_OUTX_H_G   (0x23U)
#define LSM6DSO_OUTY_L_G   (0x24U)
#define LSM6DSO_OUTY_H_G   (0x25U)
#define LSM6DSO_OUTZ_L_G   (0x26U)
#define LSM6DSO_OUTZ_H_G   (0x27U)
#define LSM6DSO_OUTX_L_XL  (0x28U)
#define LSM6DSO_OUTX_H_XL  (0x29U)
#define LSM6DSO_OUTY_L_XL  (0x2AU)
#define LSM6DSO_OUTY_H_XL  (0x2BU)
#define LSM6DSO_OUTZ_L_XL  (0x2CU)
#define LSM6DSO_OUTZ_H_XL  (0x2DU)

// LSM6DSO Configuration Values
#define LSM6DSO_EXPECTED_ID        0x6CU
#define LSM6DSO_GYRO_208HZ_2000DPS 0x5CU // ODR=208Hz, FS=2000 dps (FS_G=11)
#define LSM6DSO_BDU_ENABLE         0x40U // Block Data Update enable
// Sensitivity for FS=2000 dps
#define LSM6DSO_GYRO_SENSITIVITY_MDPS  (70U) // 70 mdps/LSB
#define LSM6DSO_GYRO_DPSx100_PER_LSB   (7)   // 0.07 dps/LSB -> dps*100 per LSB

// LSM6DSO Status Register Bits
#define LSM6DSO_STATUS_XLDA 0x01U // Accelerometer data available
#define LSM6DSO_STATUS_GDA  0x02U // Gyroscope data available
#define LSM6DSO_STATUS_TDA  0x04U // Temperature data available

// General purpose
// Bit Masks for 32-bit Registers
#define BIT_MASK(n) (1U << ((n) & 0x1FU))

// Function Prototypes
void init_clocks(void);
void init_gpio(void);
void init_spi(void);
void init_uart(void);
void init_timer(void);

void delay_ms(u32 ms);

int spi_write(u8 reg, u8 data);
int spi_read(u8 reg, u8 *data);
int spi_read_burst(u8 reg, u8 *buffer, u8 len);

int spi_write_fixed(u8 reg, u8 data);
int spi_read_fixed(u8 reg, u8 *data);
int spi_read_burst_fixed(u8 reg, u8 *buffer, u8 len);

int spi_read_burst_fixed_v2(u8 reg, u8 *buffer, u8 len);

void uart_puts(const char *s);
u32 get_system_tick(void);

#endif // UTILS_H