#include "hal/pwm.h"
#include "stm32f10x.h"
#include <stddef.h>

#define PWM_TIM_PSC ((SYSTEM_CLOCK_HZ / 1000000UL) - 1UL)
#define PWM_TIM_ARR (PWM_PERIOD_US - 1UL)

STATIC_ASSERT(PWM_TIM_PSC < 65536UL, "PWM timer prescaler overflow");
STATIC_ASSERT(PWM_TIM_ARR < 65536UL, "PWM timer auto-reload overflow");

static bool g_pwm_initialized = false;

void pwm_init(void) {
	if (g_pwm_initialized) {
		return;
	}

	RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST;
	RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM1RST;
	RCC->APB1RSTR |= RCC_APB1RSTR_TIM4RST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM4RST;

	for (volatile int i = 0; i < 100; i++) {
		__NOP();
	}

	TIM1->CR1 = 0UL;
	TIM1->CR2 = 0UL;
	TIM1->SMCR = 0UL;
	TIM1->DIER = 0UL;
	TIM1->SR = 0UL;
	TIM1->CNT = 0UL;
	TIM1->PSC = PWM_TIM_PSC;
	TIM1->ARR = PWM_TIM_ARR;

	TIM1->CCMR1 = 0UL;
	TIM1->CCMR2 = 0UL;
	TIM1->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
	TIM1->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
	TIM1->CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE;
	TIM1->CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE;

	TIM1->CCER = 0UL;
	TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;

	TIM1->CCR1 = PWM_PULSE_CENTER_US;
	TIM1->CCR2 = PWM_PULSE_CENTER_US;
	TIM1->CCR3 = PWM_PULSE_CENTER_US;
	TIM1->CCR4 = PWM_PULSE_CENTER_US;

	TIM1->BDTR |= TIM_BDTR_MOE;

	TIM1->EGR |= TIM_EGR_UG;

	TIM1->CR1 |= TIM_CR1_CEN;

	TIM4->CR1 = 0UL;
	TIM4->CR2 = 0UL;
	TIM4->SMCR = 0UL;
	TIM4->DIER = 0UL;
	TIM4->SR = 0UL;
	TIM4->CNT = 0UL;
	TIM4->PSC = PWM_TIM_PSC;
	TIM4->ARR = PWM_TIM_ARR;

	TIM4->CCMR1 = 0UL;
	TIM4->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
	TIM4->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;

	TIM4->CCER = 0UL;
	TIM4->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;

	TIM4->CCR1 = PWM_PULSE_CENTER_US;
	TIM4->CCR2 = PWM_PULSE_CENTER_US;

	TIM4->EGR |= TIM_EGR_UG;

	TIM4->CR1 |= TIM_CR1_CEN;

	g_pwm_initialized = true;
}

void pwm_deinit(void) {
	TIM1->CR1 &= ~TIM_CR1_CEN;
	TIM4->CR1 &= ~TIM_CR1_CEN;
	TIM1->CCER = 0UL;
	TIM4->CCER = 0UL;
	g_pwm_initialized = false;
}

pwm_status_t pwm_set_pulse_us(pwm_channel_t channel, u16 pulse_us) {
	if (!g_pwm_initialized) {
		return PWM_ERR_NOT_INITIALIZED;
	}

	if (pulse_us < PWM_PULSE_MIN_US || pulse_us > PWM_PULSE_MAX_US) {
		return PWM_ERR_INVALID_PULSE;
	}

	switch (channel) {
	case PWM_CH_TIM1_CH1:
		TIM1->CCR1 = pulse_us;
		break;
	case PWM_CH_TIM1_CH2:
		TIM1->CCR2 = pulse_us;
		break;
	case PWM_CH_TIM1_CH3:
		TIM1->CCR3 = pulse_us;
		break;
	case PWM_CH_TIM1_CH4:
		TIM1->CCR4 = pulse_us;
		break;
	case PWM_CH_TIM4_CH1:
		TIM4->CCR1 = pulse_us;
		break;
	case PWM_CH_TIM4_CH2:
		TIM4->CCR2 = pulse_us;
		break;
	default:
		return PWM_ERR_INVALID_CHANNEL;
	}

	return PWM_OK;
}

pwm_status_t pwm_set_angle(pwm_channel_t channel, u8 angle) {
	if (angle > 180U) {
		angle = 180U;
	}

	u32 pulse = PWM_PULSE_MIN_US +
	            ((u32)angle * (PWM_PULSE_MAX_US - PWM_PULSE_MIN_US)) / 180U;

	return pwm_set_pulse_us(channel, (u16)pulse);
}

pwm_status_t pwm_set_pulse_all(const u16 *pulse_us) {
	if (!g_pwm_initialized) {
		return PWM_ERR_NOT_INITIALIZED;
	}

	if (pulse_us == NULL) {
		return PWM_ERR_INVALID_CHANNEL;
	}

	for (u8 i = 0U; i < PWM_CHANNELS_COUNT; i++) {
		u16 pulse = pulse_us[i];
		if (pulse < PWM_PULSE_MIN_US) {
			pulse = PWM_PULSE_MIN_US;
		} else if (pulse > PWM_PULSE_MAX_US) {
			pulse = PWM_PULSE_MAX_US;
		}

		pwm_set_pulse_us((pwm_channel_t)i, pulse);
	}

	return PWM_OK;
}

bool pwm_is_enabled(void) {
	return g_pwm_initialized && ((TIM1->CR1 & TIM_CR1_CEN) != 0U) &&
	       ((TIM4->CR1 & TIM_CR1_CEN) != 0U);
}
