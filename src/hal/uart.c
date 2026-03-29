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
	volatile u16 write_idx;
	volatile u16 read_idx;
} g_uart_tx = {{0}, 0U, 0U};

static bool g_uart_initialized = false;

void uart_init(void) {
	if (g_uart_initialized) {
		return;
	}
	RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_USART2RST;

	USART2->CR1 &= ~USART_CR1_UE;

	USART2->BRR = UART_BRR;

	USART2->CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS);

	USART2->CR2 &= ~USART_CR2_STOP;

	USART2->CR3 &= ~(USART_CR3_RTSE | USART_CR3_CTSE);

	USART2->SR = 0UL;

	USART2->CR1 &= ~(USART_CR1_RXNEIE | USART_CR1_TCIE | USART_CR1_TXEIE |
	                 USART_CR1_PEIE | USART_CR1_IDLEIE);

	USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);

	USART2->CR1 |= USART_CR1_UE;

	u32 timeout = 1000U;
	while (((USART2->SR & USART_SR_TC) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}

	volatile u32 dummy_sr = USART2->SR;
	USART2->DR = UART_DUMMY_BYTE;
	(void)dummy_sr;

	NVIC_SetPriority(USART2_IRQn, 2U);
	NVIC_EnableIRQ(USART2_IRQn);

	g_uart_initialized = true;
}

bool uart_is_enabled(void) { return ((USART2->CR1 & USART_CR1_UE) != 0U); }

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
		const u16 next_idx = (g_uart_tx.write_idx + 1U) % UART_TX_BUFFER_SIZE;

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

	if ((chars_added > 0U) && ((USART2->CR1 & USART_CR1_TXEIE) == 0U)) {
		USART2->CR1 |= USART_CR1_TXEIE;
	}

	__enable_irq();

	return (chars_lost > 0U) ? UART_ERR_BUFFER_FULL : UART_OK;
}

void uart_deinit(void) {
	NVIC_DisableIRQ(USART2_IRQn);
	USART2->CR1 &= ~USART_CR1_UE;
	g_uart_tx.write_idx = 0U;
	g_uart_tx.read_idx = 0U;
}

void uart_clear_errors(void) {
	const u32 sr = USART2->SR;
	if ((sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0U) {
		volatile u32 dummy = USART2->DR;
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

void USART2_IRQHandler(void) {
	const u32 sr = USART2->SR;

	if (((USART2->CR1 & USART_CR1_TXEIE) != 0U) && ((sr & USART_SR_TXE) != 0U)) {
		if (g_uart_tx.read_idx != g_uart_tx.write_idx) {
			USART2->DR = (u16)g_uart_tx.buffer[g_uart_tx.read_idx];
			g_uart_tx.read_idx = (g_uart_tx.read_idx + 1U) % UART_TX_BUFFER_SIZE;
		} else {
			USART2->CR1 &= ~USART_CR1_TXEIE;
		}
	}

	if ((sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0U) {
		volatile u32 dummy = USART2->DR;
		(void)dummy;
	}

	if ((sr & USART_SR_RXNE) != 0U) {
		volatile u8 received_data = (u8)USART2->DR;
		(void)received_data;
	}
}
