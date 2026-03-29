/**
 * @file clock.h
 * @brief System clock configuration module
 * @author STM32_LSM6DSO Project
 *
 * This module handles RCC configuration, PLL setup, and peripheral clock
 * enable.
 */

#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

#include <stdint.h>

/**
 * @brief Clock initialization status codes
 */
typedef enum {
	CLOCK_OK = 0,               /**< Initialization successful */
	CLOCK_ERR_HSE_TIMEOUT = -1, /**< HSE oscillator startup timeout */
	CLOCK_ERR_PLL_TIMEOUT = -2  /**< PLL lock timeout */
} clock_status_t;

/**
 * @brief Initialize system clock
 *
 * Configures the system clock using HSE and PLL.
 * On error, falls back to HSI and returns error code.
 *
 * @return CLOCK_OK on success, error code on failure
 *
 * @note Enables clocks for GPIOA, SPI1, USART2, TIM1, TIM2, TIM3, TIM4
 */
clock_status_t clock_init(void);

#endif
