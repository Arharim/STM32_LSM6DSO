/**
 * @file config.h
 * @brief System configuration constants
 * @author STM32_LSM6DSO Project
 *
 * This header contains all system-wide configuration constants
 * including clock settings, peripheral frequencies, and helper macros.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/*============================================================================
 * Version Information
 *===========================================================================*/
#define FW_VERSION_MAJOR  (1U) /**< Major version (API changes) */
#define FW_VERSION_MINOR  (0U) /**< Minor version (features) */
#define FW_VERSION_PATCH  (0U) /**< Patch version (bug fixes) */
#define FW_VERSION_STRING "1.0.0"

/*============================================================================
 * Feature Flags
 *===========================================================================*/
#define DEBUG_LOG_ENABLED (0U) /**< Enable debug logging via UART */

/*============================================================================
 * Clock Configuration
 *===========================================================================*/
#define SYSTEM_CLOCK_HZ (24000000UL) /**< System clock frequency (24 MHz) */
#define HSE_VALUE_HZ    (8000000UL)  /**< External crystal frequency (8 MHz) */

/*============================================================================
 * UART Configuration
 *===========================================================================*/
#define UART_BAUD_RATE (115200UL) /**< UART baud rate */

/*============================================================================
 * Timer Configuration
 *===========================================================================*/
#define TIMER_FREQ_HZ       (100UL)  /**< TIM2 frequency (100 Hz) */
#define DELAY_TIMER_FREQ_HZ (1000UL) /**< TIM3 frequency (1 kHz) */

/*============================================================================
 * Startup Timeouts
 *===========================================================================*/
#define HSE_STARTUP_TIMEOUT (0x5000U) /**< HSE startup timeout */
#define PLL_LOCK_TIMEOUT    (0x5000U) /**< PLL lock timeout */

/*============================================================================
 * Watchdog Configuration
 *===========================================================================*/
#define IWDG_TIMEOUT_MS (1000U) /**< Watchdog timeout in milliseconds */

/*============================================================================
 * Derived Values (auto-calculated)
 *===========================================================================*/
#define UART_BRR ((SYSTEM_CLOCK_HZ + (UART_BAUD_RATE / 2U)) / UART_BAUD_RATE)

#define TIM2_PSC ((SYSTEM_CLOCK_HZ / 100000UL) - 1UL)
#define TIM2_ARR ((100000UL / TIMER_FREQ_HZ) - 1UL)
#define TIM3_PSC ((SYSTEM_CLOCK_HZ / 1000000UL) - 1UL)
#define TIM3_ARR (1000UL - 1UL)

#define IWDG_RELOAD ((40000UL * IWDG_TIMEOUT_MS) / 1000UL)

/*============================================================================
 * Helper Macros
 *===========================================================================*/
#define BIT_MASK(n) (1U << ((n) & 0x1FU))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/*============================================================================
 * Compile-time Validation
 *===========================================================================*/
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#define STATIC_ASSERT(cond, msg)                                               \
	typedef char static_assert_##msg[(cond) ? 1 : -1]
#endif

STATIC_ASSERT(SYSTEM_CLOCK_HZ >= 1000000UL, "System clock must be >= 1 MHz");
STATIC_ASSERT(SYSTEM_CLOCK_HZ <= 72000000UL, "System clock must be <= 72 MHz");
STATIC_ASSERT(UART_BAUD_RATE >= 9600UL, "UART baud rate must be >= 9600");
STATIC_ASSERT(TIMER_FREQ_HZ >= 1UL, "Timer frequency must be >= 1 Hz");
STATIC_ASSERT(TIMER_FREQ_HZ <= 10000UL, "Timer frequency must be <= 10 kHz");

STATIC_ASSERT(TIM2_PSC < 65536UL, "TIM2 prescaler overflow");
STATIC_ASSERT(TIM2_ARR < 65536UL, "TIM2 auto-reload overflow");
STATIC_ASSERT(TIM3_PSC < 65536UL, "TIM3 prescaler overflow");
STATIC_ASSERT(TIM3_ARR < 65536UL, "TIM3 auto-reload overflow");

#endif
