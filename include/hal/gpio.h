/**
 * @file gpio.h
 * @brief GPIO configuration module
 * @author STM32_LSM6DSO Project
 *
 * This module handles GPIO initialization for SPI, UART, and unused pins.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>

/**
 * @brief Initialize GPIO peripherals
 *
 * Configures:
 * - PA0-3, PA8, PA11-15: Input with pull-down
 * - PA4 (SPI_CS): Output push-pull, initially high
 * - PA5 (SPI_SCK): Alternate function push-pull
 * - PA6 (SPI_MISO): Input floating
 * - PA7 (SPI_MOSI): Alternate function push-pull
 * - PA9 (UART_TX): Alternate function push-pull
 * - PA10 (UART_RX): Input floating
 * - GPIOB, GPIOC: All pins input with pull-down
 */
void gpio_init(void);

/**
 * @brief Deinitialize GPIO peripherals
 *
 * Disables GPIOA, GPIOB, GPIOC clocks.
 */
void gpio_deinit(void);

#endif
