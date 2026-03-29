#include "hal/spi.h"
#include "config.h"
#include "hal/gpio_config.h"
#include "stm32f10x.h"
#include <stddef.h>

#define SPI_DELAY_LOOP_COUNT (10U)

static bool g_spi_initialized = false;

static void spi_clear_rx_buffer(void) {
	while ((SPI1->SR & SPI_SR_RXNE) != 0U) {
		volatile u8 dummy = (u8)SPI1->DR;
		(void)dummy;
	}
}

void spi_init(void) {
	if (g_spi_initialized) {
		return;
	}

	RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
	RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;

	SPI1->CR1 &= ~SPI_CR1_SPE;

	SPI1->CR1 = 0UL;
	SPI1->CR1 |= SPI_CR1_MSTR;
	SPI1->CR1 |= SPI_CR1_BR_2;
	SPI1->CR1 |= SPI_CR1_CPOL;
	SPI1->CR1 |= SPI_CR1_CPHA;
	SPI1->CR1 |= SPI_CR1_SSM;
	SPI1->CR1 |= SPI_CR1_SSI;

	SPI1->CR2 = 0UL;

	volatile u32 dummy = SPI1->DR;
	dummy = SPI1->SR;
	(void)dummy;

	SPI1->CR1 |= SPI_CR1_SPE;

	u32 timeout = 1000UL;
	while (((SPI1->CR1 & SPI_CR1_SPE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}

	spi_cs_high();
	g_spi_initialized = true;
}

void spi_deinit(void) {
	SPI1->CR1 &= ~SPI_CR1_SPE;
	NVIC_DisableIRQ(SPI1_IRQn);
	spi_cs_high();
}

void spi_cs_high(void) { GPIOA->BSRR = BIT_MASK(SPI_CS_PIN); }

void spi_cs_low(void) { GPIOA->BSRR = (BIT_MASK(SPI_CS_PIN) << 16U); }

spi_status_t spi_write_byte(u8 reg, u8 data) {
	u32 timeout;

	spi_clear_rx_buffer();
	spi_cs_low();

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	SPI1->DR = (u16)reg;

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	volatile u8 dummy1 = (u8)SPI1->DR;
	(void)dummy1;

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	SPI1->DR = (u16)data;

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	volatile u8 dummy2 = (u8)SPI1->DR;
	(void)dummy2;

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}

	spi_cs_high();

	for (volatile int i = 0; i < SPI_DELAY_LOOP_COUNT; i++) {
		__NOP();
	}

	return SPI_OK;
}

spi_status_t spi_read_byte(u8 reg, u8 *data) {
	u32 timeout;

	if (data == NULL) {
		return SPI_ERR_PARAM;
	}

	spi_clear_rx_buffer();
	spi_cs_low();

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	SPI1->DR = (u16)(reg | 0x80U);

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	(void)SPI1->DR;

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	SPI1->DR = (u16)SPI_DUMMY_BYTE;

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	*data = (u8)SPI1->DR;

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) {
		timeout--;
	}

	spi_cs_high();

	for (volatile int i = 0; i < SPI_DELAY_LOOP_COUNT; i++) {
		__NOP();
	}

	return SPI_OK;
}

spi_status_t spi_read_burst(u8 reg, u8 *buffer, u8 len) {
	u32 timeout;

	if ((buffer == NULL) || (len == 0U)) {
		return SPI_ERR_PARAM;
	}

	spi_clear_rx_buffer();
	spi_cs_low();

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	SPI1->DR = (u16)(reg | 0x80U);

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}
	if (timeout == 0U) {
		spi_cs_high();
		return SPI_ERR_TIMEOUT;
	}
	volatile u8 dummy = (u8)SPI1->DR;
	(void)dummy;

	for (u8 i = 0U; i < len; i++) {
		timeout = SPI_TIMEOUT_MS;
		while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
			timeout--;
			__NOP();
		}
		if (timeout == 0U) {
			spi_cs_high();
			return SPI_ERR_TIMEOUT;
		}
		SPI1->DR = (u16)SPI_DUMMY_BYTE;

		timeout = SPI_TIMEOUT_MS;
		while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
			timeout--;
			__NOP();
		}
		if (timeout == 0U) {
			spi_cs_high();
			return SPI_ERR_TIMEOUT;
		}
		buffer[i] = (u8)SPI1->DR;
	}

	timeout = SPI_TIMEOUT_MS;
	while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) {
		timeout--;
		__NOP();
	}

	spi_cs_high();

	for (volatile int i = 0; i < SPI_DELAY_LOOP_COUNT; i++) {
		__NOP();
	}

	return SPI_OK;
}
