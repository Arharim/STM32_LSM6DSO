#include "hal/timer.h"
#include "config.h"
#include "stm32f10x.h"

static volatile u32 g_system_tick = 0U;
static bool g_timer_initialized = false;

void timer_init(void) {
	if (g_timer_initialized) {
		return;
	}
	u32 timeout;
	u16 initial_count;

	RCC->APB1RSTR |= (RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST);
	RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST);

	for (volatile int i = 0; i < 100; i++) {
		__NOP();
	}

	TIM2->CR1 = 0UL;
	TIM2->CR2 = 0UL;
	TIM2->SMCR = 0UL;
	TIM2->DIER = 0UL;
	TIM2->SR = 0UL;
	TIM2->CNT = 0UL;

	TIM2->PSC = TIM2_PSC;
	TIM2->ARR = TIM2_ARR;

	TIM2->EGR |= TIM_EGR_UG;

	TIM2->SR &= ~TIM_SR_UIF;

	TIM2->DIER |= TIM_DIER_UIE;

	NVIC_SetPriority(TIM2_IRQn, 3U);
	NVIC_EnableIRQ(TIM2_IRQn);

	TIM2->CR1 |= TIM_CR1_CEN;

	timeout = TIMER_CONFIG_TIMEOUT;
	initial_count = TIM2->CNT;
	while ((TIM2->CNT == initial_count) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}

	TIM3->CR1 = 0UL;
	TIM3->CR2 = 0UL;
	TIM3->SMCR = 0UL;
	TIM3->DIER = 0UL;
	TIM3->SR = 0UL;
	TIM3->CNT = 0UL;

	TIM3->PSC = TIM3_PSC;
	TIM3->ARR = TIM3_ARR;

	TIM3->EGR |= TIM_EGR_UG;

	TIM3->SR &= ~TIM_SR_UIF;

	TIM3->CR1 |= TIM_CR1_CEN;

	timeout = TIMER_CONFIG_TIMEOUT;
	initial_count = TIM3->CNT;
	while ((TIM3->CNT == initial_count) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}

	g_timer_initialized = true;
}

void timer_deinit(void) {
	NVIC_DisableIRQ(TIM2_IRQn);
	TIM2->CR1 &= ~TIM_CR1_CEN;
	TIM3->CR1 &= ~TIM_CR1_CEN;
	g_system_tick = 0U;
}

void delay_ms(u32 ms) {
	u32 timeout;

	if (ms == 0U) {
		return;
	}

	if (!timer_is_running((u32)TIM3)) {
		for (volatile u32 i = 0U; i < (ms * (SYSTEM_CLOCK_HZ / 1000UL)); i++) {
			__NOP();
		}
		return;
	}

	for (u32 i = 0U; i < ms; i++) {
		TIM3->CNT = 0UL;

		timeout = 10000U;
		while ((TIM3->CNT == 0U) && (timeout > 0U)) {
			timeout--;
		}

		while ((TIM3->CNT < 1U) && (timeout > 0U)) {
			timeout--;
		}

		if (timeout == 0U) {
			break;
		}
	}
}

u32 get_system_tick(void) {
	u32 tick_value;
	__disable_irq();
	tick_value = g_system_tick;
	__enable_irq();
	return tick_value;
}

bool timer_is_running(u32 timer_base) {
	bool is_running = false;

	if (timer_base == (u32)TIM2) {
		is_running = ((TIM2->CR1 & TIM_CR1_CEN) != 0U);
	} else if (timer_base == (u32)TIM3) {
		is_running = ((TIM3->CR1 & TIM_CR1_CEN) != 0U);
	}

	return is_running;
}

void SysTick_Handler(void) { g_system_tick++; }

void TIM2_IRQHandler(void) {
	if ((TIM2->SR & TIM_SR_UIF) != 0U) {
		TIM2->SR &= ~TIM_SR_UIF;
	}
}
