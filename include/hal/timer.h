/**
 * @file timer.h
 * @brief Timer and delay module
 * @author STM32_LSM6DSO Project
 *
 * This module provides system tick, millisecond delays, and timer
 * configuration. Uses SysTick for system time and TIM2/TIM3 for delays.
 */

#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include "config.h"
#include <stdbool.h>

#define TIMER_CONFIG_TIMEOUT                                                   \
	(10000U) /**< Timer configuration timeout in loops */

/**
 * @brief Initialize timers
 *
 * Configures:
 * - SysTick: 1ms interrupts for system tick
 * - TIM2: 100Hz periodic timer
 * - TIM3: 1kHz for delay functions
 */
void timer_init(void);

/**
 * @brief Deinitialize timers
 *
 * Stops TIM2, TIM3 and resets system tick counter.
 */
void timer_deinit(void);

/**
 * @brief Blocking delay in milliseconds
 *
 * Uses TIM3 for precise delays. Falls back to busy-wait
 * if TIM3 is not running.
 *
 * @param ms Delay duration in milliseconds
 */
void delay_ms(u32 ms);

/**
 * @brief Get system tick counter
 *
 * Returns milliseconds since system start.
 *
 * @return System tick count in milliseconds
 */
u32 get_system_tick(void);

/**
 * @brief Check if a timer is running
 *
 * @param timer_base Timer base address (TIM2 or TIM3)
 * @return true if timer is enabled, false otherwise
 */
bool timer_is_running(u32 timer_base);

#endif
