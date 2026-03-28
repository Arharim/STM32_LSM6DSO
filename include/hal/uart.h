/**
 * @file uart.h
 * @brief UART driver module
 * @author STM32_LSM6DSO Project
 *
 * This module provides UART communication using USART1 with
 * interrupt-driven TX circular buffer.
 */

#ifndef HAL_UART_H
#define HAL_UART_H

#include "config.h"
#include <stdbool.h>
#include <stdint.h>

#define UART_TX_BUFFER_SIZE (256U)  /**< TX circular buffer size */
#define UART_DUMMY_BYTE     (0x00U) /**< Dummy byte for initialization */

STATIC_ASSERT(UART_TX_BUFFER_SIZE >= 64U, "UART TX buffer too small");
STATIC_ASSERT(UART_TX_BUFFER_SIZE <= 4096U, "UART TX buffer too large");
STATIC_ASSERT((UART_TX_BUFFER_SIZE & (UART_TX_BUFFER_SIZE - 1U)) == 0U,
              "UART TX buffer size must be power of 2");

/**
 * @brief UART operation status codes
 */
typedef enum {
	UART_OK = 0,              /**< Operation successful */
	UART_ERR_BUFFER_FULL = -1 /**< TX buffer full, some data lost */
} uart_status_t;

/**
 * @brief Initialize USART1 peripheral
 *
 * Configures USART1 as:
 * - 115200 baud
 * - 8 data bits
 * - No parity
 * - 1 stop bit
 * - TX interrupt-driven with circular buffer
 */
void uart_init(void);

/**
 * @brief Deinitialize USART1 peripheral
 *
 * Disables USART1, clears TX buffer, disables interrupts.
 */
void uart_deinit(void);

/**
 * @brief Send string over UART (non-blocking)
 *
 * Adds string to TX buffer. If buffer is full, remaining
 * characters are silently discarded.
 *
 * @param s Null-terminated string to send
 */
void uart_puts(const char *s);

/**
 * @brief Send string over UART with status (non-blocking)
 *
 * Adds string to TX buffer. If buffer is full, remaining
 * characters are discarded and error is returned.
 *
 * @param s Null-terminated string to send
 * @return UART_OK on success, UART_ERR_BUFFER_FULL if data lost
 */
uart_status_t uart_puts_safe(const char *s);

/**
 * @brief Send formatted debug message (conditional)
 *
 * Only compiled when DEBUG_LOG_ENABLED is 1.
 *
 * @param fmt Format string
 * @param ... Variable arguments
 */
#if DEBUG_LOG_ENABLED
void uart_debug_printf(const char *fmt, ...);
#else
#define uart_debug_printf(fmt, ...) ((void)0)
#endif

/**
 * @brief Check if UART is enabled
 *
 * @return true if UART is enabled, false otherwise
 */
bool uart_is_enabled(void);

/**
 * @brief Clear UART error flags
 *
 * Clears ORE (overrun), NE (noise), FE (framing), PE (parity) error flags
 * by reading DR register.
 */
void uart_clear_errors(void);

#endif
