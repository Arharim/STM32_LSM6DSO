/**
 * @file pwm.h
 * @brief PWM driver for servo motors
 * @author STM32_LSM6DSO Project
 *
 * This module provides PWM generation using TIM1 and TIM4 for servo control.
 * Supports 6 channels: PA8-PA11 (TIM1) and PB6-PB7 (TIM4).
 */

#ifndef HAL_PWM_H
#define HAL_PWM_H

#include "config.h"
#include <stdbool.h>

#define PWM_FREQ_HZ   (50U)    /**< PWM frequency for servos (50 Hz) */
#define PWM_PERIOD_US (20000U) /**< PWM period in microseconds (20 ms) */

#define PWM_PULSE_MIN_US (1000U) /**< Minimum pulse width (1 ms, 0 degrees) */
#define PWM_PULSE_MAX_US (2000U) /**< Maximum pulse width (2 ms, 180 degrees)  \
	                                */
#define PWM_PULSE_CENTER_US                                                    \
	(1500U) /**< Center pulse width (1.5 ms, 90 degrees) */

#define PWM_CHANNELS_COUNT (6U) /**< Total number of PWM channels */

typedef enum {
	PWM_CH_TIM1_CH1 = 0, /**< PA8 - TIM1 Channel 1 */
	PWM_CH_TIM1_CH2 = 1, /**< PA9 - TIM1 Channel 2 */
	PWM_CH_TIM1_CH3 = 2, /**< PA10 - TIM1 Channel 3 */
	PWM_CH_TIM1_CH4 = 3, /**< PA11 - TIM1 Channel 4 */
	PWM_CH_TIM4_CH1 = 4, /**< PB6 - TIM4 Channel 1 */
	PWM_CH_TIM4_CH2 = 5, /**< PB7 - TIM4 Channel 2 */
} pwm_channel_t;

typedef enum {
	PWM_OK = 0,                   /**< Operation successful */
	PWM_ERR_INVALID_CHANNEL = -1, /**< Invalid channel */
	PWM_ERR_INVALID_PULSE = -2,   /**< Pulse width out of range */
	PWM_ERR_NOT_INITIALIZED = -3  /**< PWM not initialized */
} pwm_status_t;

void pwm_init(void);

void pwm_deinit(void);

pwm_status_t pwm_set_pulse_us(pwm_channel_t channel, u16 pulse_us);

pwm_status_t pwm_set_angle(pwm_channel_t channel, u8 angle);

pwm_status_t pwm_set_pulse_all(const u16 *pulse_us);

bool pwm_is_enabled(void);

#endif
