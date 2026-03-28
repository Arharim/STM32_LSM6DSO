/**
 * @file fsm.h
 * @brief Finite State Machine for sensor data acquisition
 * @author STM32_LSM6DSO Project
 *
 * This module implements a non-blocking FSM for:
 * - System initialization
 * - Sensor stabilization
 * - Data acquisition
 * - Data formatting and output
 */

#ifndef APP_FSM_H
#define APP_FSM_H

#include "drivers/lsm6dso.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief FSM state enumeration
 */
typedef enum {
	FSM_STATE_INIT,        /**< Initialize hardware and sensor */
	FSM_STATE_STABILIZING, /**< Wait for sensor stabilization */
	FSM_STATE_READ,        /**< Read sensor data */
	FSM_STATE_PROCESS,     /**< Format data for output */
	FSM_STATE_OUTPUT,      /**< Send data via UART */
	FSM_STATE_IDLE,        /**< Wait for next sample (WFI) */
	FSM_STATE_ERROR        /**< Error state (halted) */
} fsm_state_t;

/**
 * @brief FSM context structure
 */
typedef struct {
	volatile fsm_state_t current_state; /**< Current FSM state */
	lsm6dso_data_t sensor_data;         /**< Latest sensor data */
	uint8_t retry_count;                /**< ID check retry counter */
	uint32_t stabilization_start;       /**< Stabilization start timestamp */
	uint32_t last_retry_time;           /**< Last ID check timestamp */
} fsm_context_t;

/**
 * @brief Initialize FSM
 *
 * Sets initial state and resets counters.
 */
void fsm_init(void);

/**
 * @brief Execute one FSM cycle
 *
 * Should be called in main loop. Uses __WFI() in IDLE state
 * for power saving.
 */
void fsm_run(void);

/**
 * @brief Set FSM state (thread-safe)
 *
 * @param new_state State to transition to
 */
void fsm_set_state(fsm_state_t new_state);

/**
 * @brief Get current FSM state (thread-safe)
 *
 * @return Current FSM state
 */
fsm_state_t fsm_get_state(void);

#endif
