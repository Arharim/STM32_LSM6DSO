/**
 * @file watchdog.h
 * @brief Independent Watchdog (IWDG) module
 * @author STM32_LSM6DSO Project
 *
 * This module provides watchdog timer functionality for system
 * recovery from software failures.
 */

#ifndef HAL_WATCHDOG_H
#define HAL_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize Independent Watchdog
 *
 * Configures IWDG with timeout defined by IWDG_TIMEOUT_MS.
 * The watchdog starts automatically and must be refreshed
 * periodically using watchdog_refresh().
 */
void watchdog_init(void);

/**
 * @brief Refresh watchdog timer
 *
 * Must be called periodically before timeout expires.
 * Typically called in main loop or IDLE state.
 */
void watchdog_refresh(void);

/**
 * @brief Check if watchdog is enabled
 *
 * @return true if watchdog is active
 */
bool watchdog_is_enabled(void);

#endif
