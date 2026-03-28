#include "hal/uart.h"
#include "config.h"
#include "stm32f10x.h"
#include <stddef.h>
#if DEBUG_LOG_ENABLED
#include <stdarg.h>
#include <stdio.h>
#endif

static volatile struct {
	char buffer[UART_TX_BUFFER_SIZE];
	volatile uint16_t write_idx;
	volatile uint16_t read_idx;
} g_uart_tx = {{0}, 0U, 0U};

static bool g_uart_initialized = false;

void uart_init(void) {
	if (g_uart_initialized) {
		return;
	}
	RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
	RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;

	USART1->CR1 &= ~USART_CR1_UE;

	USART1->BRR = UART_BRR;

	USART1->CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS);

	USART1->CR2 &= ~USART_CR2_STOP;

	USART1->CR3 &= ~(USART_CR3_RTSE | USART_CR3_CTSE);

	USART1->SR = 0UL;

	USART1->CR1 &= ~(USART_CR1_RXNEIE | USART_CR1_TCIE | USART_CR1_TXEIE |
	                 USART_CR1_PEIE | USART_CR1_IDLEIE);

	USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE);

	USART1->CR1 |= USART_CR1_UE;

	uint32_t timeout = 1000U;
	while (((USART1->SR & USART_SR_TC) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}

	volatile uint32_t dummy_sr = USART1->SR;
	USART1->DR = UART_DUMMY_BYTE;
	(void)dummy_sr;

	NVIC_SetPriority(USART1_IRQn, 2U);
	NVIC_EnableIRQ(USART1_IRQn);

	g_uart_initialized = true;
}

bool uart_is_enabled(void) { return ((USART1->CR1 & USART_CR1_UE) != 0U); }

void uart_puts(const char *const s) { (void)uart_puts_safe(s); }

uart_status_t uart_puts_safe(const char *const s) {
	size_t chars_added = 0U;
	size_t chars_lost = 0U;
	const char *str_ptr = s;

	if ((s == NULL) || !uart_is_enabled()) {
		return UART_OK;
	}

	__disable_irq();

	while (*str_ptr != '\0') {
		const uint16_t next_idx = (g_uart_tx.write_idx + 1U) % UART_TX_BUFFER_SIZE;

		if (next_idx == g_uart_tx.read_idx) {
			chars_lost++;
			str_ptr++;
			continue;
		}

		g_uart_tx.buffer[g_uart_tx.write_idx] = *str_ptr;
		g_uart_tx.write_idx = next_idx;
		str_ptr++;
		chars_added++;
	}

	if ((chars_added > 0U) && ((USART1->CR1 & USART_CR1_TXEIE) == 0U)) {
		USART1->CR1 |= USART_CR1_TXEIE;
	}

	__enable_irq();

	return (chars_lost > 0U) ? UART_ERR_BUFFER_FULL : UART_OK;
}

void uart_deinit(void) {
	NVIC_DisableIRQ(USART1_IRQn);
	USART1->CR1 &= ~USART_CR1_UE;
	g_uart_tx.write_idx = 0U;
	g_uart_tx.read_idx = 0U;
}

void uart_clear_errors(void) {
	const uint32_t sr = USART1->SR;
	if ((sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0U) {
		volatile uint32_t dummy = USART1->DR;
		(void)dummy;
	}
}

#if DEBUG_LOG_ENABLED
void uart_debug_printf(const char *fmt, ...) {
	char debug_buf[128U];
	va_list args;

	va_start(args, fmt);
	int result = vsnprintf(debug_buf, sizeof(debug_buf), fmt, args);
	va_end(args);

	if ((result > 0) && ((size_t)result < sizeof(debug_buf))) {
		uart_puts(debug_buf);
	}
}
#endif

void USART1_IRQHandler(void) {
	const uint32_t sr = USART1->SR;

	if (((USART1->CR1 & USART_CR1_TXEIE) != 0U) && ((sr & USART_SR_TXE) != 0U)) {
		if (g_uart_tx.read_idx != g_uart_tx.write_idx) {
			USART1->DR = (uint16_t)g_uart_tx.buffer[g_uart_tx.read_idx];
			g_uart_tx.read_idx = (g_uart_tx.read_idx + 1U) % UART_TX_BUFFER_SIZE;
		} else {
			USART1->CR1 &= ~USART_CR1_TXEIE;
		}
	}

	if ((sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0U) {
		volatile uint32_t dummy = USART1->DR;
		(void)dummy;
	}

	if ((sr & USART_SR_RXNE) != 0U) {
		volatile uint8_t received_data = (uint8_t)USART1->DR;
		(void)received_data;
	}
}
