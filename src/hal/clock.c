#include "hal/clock.h"
#include "config.h"
#include "stm32f10x.h"

static bool g_clock_initialized = false;

clock_status_t clock_init(void) {
	if (g_clock_initialized) {
		return CLOCK_OK;
	}
	RCC->CR |= RCC_CR_HSION;
	while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {
	}

	RCC->CFGR = 0x00000000UL;
	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);
	RCC->CR &= ~RCC_CR_HSEBYP;
	RCC->CIR = 0x00000000UL;

	RCC->CR |= RCC_CR_HSEON;
	u32 timeout = HSE_STARTUP_TIMEOUT;
	while (((RCC->CR & RCC_CR_HSERDY) == 0U) && (timeout > 0U)) {
		timeout--;
	}

	if ((RCC->CR & RCC_CR_HSERDY) == 0U) {
		RCC->CR |= RCC_CR_HSION;
		while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {
		}
		RCC->CFGR &= ~RCC_CFGR_SW;
		RCC->CFGR |= RCC_CFGR_SW_HSI;
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
		}
		return CLOCK_ERR_HSE_TIMEOUT;
	}

	FLASH->ACR |= FLASH_ACR_PRFTBE;
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	FLASH->ACR |= FLASH_ACR_LATENCY_0;

	RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
	RCC->CFGR |= RCC_CFGR_PLLSRC;
	RCC->CFGR |= RCC_CFGR_PLLMULL3;

	RCC->CR |= RCC_CR_PLLON;
	timeout = PLL_LOCK_TIMEOUT;
	while (((RCC->CR & RCC_CR_PLLRDY) == 0U) && (timeout > 0U)) {
		timeout--;
	}

	if ((RCC->CR & RCC_CR_PLLRDY) == 0U) {
		return CLOCK_ERR_PLL_TIMEOUT;
	}

	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
	}

	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000U);

	RCC->APB2ENR |=
	    (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_TIM1EN);
	RCC->APB1ENR |= (RCC_APB1ENR_USART2EN | RCC_APB1ENR_TIM2EN |
	                 RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM4EN);

	g_clock_initialized = true;
	return CLOCK_OK;
}
