// #ifndef MAIN_H
// #define MAIN_H

// #include <stdint.h>
// #include <stddef.h>
// #include "stm32f10x.h"
// #include "utils.h" // Assuming this contains SYSTEM_CLOCK_HZ, UART_BRR, etc.

// // LSM6DSO Register Definitions
// #define LSM6DSO_WHO_AM_I    (0x0FU) // Chip ID register, ret 0x6C
// #define LSM6DSO_CTRL2_G     (0x11U) // Gyro control register
// #define LSM6DSO_CTRL3_C     (0x12U) // Ctrl register 3 (BDU, etc.)
// #define LSM6DSO_OUTX_L_G    (0x22U) // Gyro X-axis data LSB (start of 6-byte burst)
// #define LSM6DSO_ID_VAL      (0x6CU) // Expected chip ID

// // LSM6DSO Configuration Values
// #define LSM6DSO_GYRO_CONFIG (0x4CU) // 208 Hz, 2000 dps
// #define LSM6DSO_CTRL_CONFIG (0x40U) // BDU enabled

// // SPI Read/Write Flags
// #define SPI_READ_FLAG       (0x80U)
// #define SPI_WRITE_FLAG      (0x00U)

// // System Timing and Delays (in milliseconds)
// #define LSM6DSO_STABILIZATION_MS (100U)
// #define LSM6DSO_RETRY_DELAY_MS   (10U)
// #define MAX_INIT_RETRIES         (3U)

// // Buffer Sizes
// #define UART_TX_BUFFER_SIZE (256U)
// #define FORMAT_BUFFER_SIZE  (64U)
// #define SPI_BUFFER_SIZE     (7U) // 1 for cmd + 6 for data

// // Timeouts
// #define SPI_UART_TIMEOUT     (1000U)
// #define TIMER_CONFIG_TIMEOUT (1000U)
// #define CLOCK_TIMEOUT        (0x5000U)

// // --- Type Definitions ---

// // FSM States
// typedef enum
// {
//    STATE_INIT,        // Init hardware and gyroscope
//    STATE_STABILIZING, // Non-blocking delay for sensor stabilization
//    STATE_READ,        // Read sensor data
//    STATE_PROCESS,     // Process data into a string
//    STATE_OUTPUT,      // Output data via UART
//    STATE_IDLE,        // Wait for timer interrupt
//    STATE_ERROR        // Error state for recovery or halt
// } State_t;

// // System State Structure
// typedef struct {
//     volatile State_t current_state;
//     volatile int16_t gyro_x;
//     volatile int16_t gyro_y;
//     volatile int16_t gyro_z;
//     uint8_t retry_count;
//     uint32_t stabilization_start_time;
//     uint32_t last_retry_time;
//     volatile uint32_t system_tick;
// } SystemState_t;

// // UART Circular Buffer
// typedef struct {
//     volatile char buffer[UART_TX_BUFFER_SIZE];
//     volatile uint16_t write_idx;
//     volatile uint16_t read_idx;
// } UartBuffer_t;


// // --- Function Prototypes ---
// // In a real project, prototypes for functions in other files would go here.
// // Since all functions are static to main.c, we don't need them here.

// #endif // MAIN_H