/**
 * @file spi.h
 * @brief SPI driver module
 * @author STM32_LSM6DSO Project
 *
 * This module provides SPI communication using SPI1 in Master Mode 3.
 */

#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <stdbool.h>
#include <stdint.h>

#define SPI_TIMEOUT_MS (1000U) /**< SPI operation timeout in milliseconds */
#define SPI_DUMMY_BYTE (0xFFU) /**< Dummy byte for read operations */

/**
 * @brief SPI operation status codes
 */
typedef enum {
	SPI_OK = 0,           /**< Operation successful */
	SPI_ERR_TIMEOUT = -1, /**< Operation timed out */
	SPI_ERR_PARAM = -2    /**< Invalid parameter */
} spi_status_t;

/**
 * @brief Initialize SPI1 peripheral
 *
 * Configures SPI1 as:
 * - Master mode
 * - Clock polarity high (CPOL=1)
 * - Clock phase second edge (CPHA=1)
 * - Baud rate: fPCLK/16
 * - Software NSS management
 */
void spi_init(void);

/**
 * @brief Deinitialize SPI1 peripheral
 *
 * Disables SPI1 and releases resources.
 */
void spi_deinit(void);

/**
 * @brief Set CS pin high (deselect slave)
 */
void spi_cs_high(void);

/**
 * @brief Set CS pin low (select slave)
 */
void spi_cs_low(void);

/**
 * @brief Write a single byte to SPI register
 *
 * @param reg Register address (7-bit)
 * @param data Data byte to write
 * @return SPI_OK on success, error code on failure
 */
spi_status_t spi_write_byte(uint8_t reg, uint8_t data);

/**
 * @brief Read a single byte from SPI register
 *
 * @param reg Register address (7-bit, bit 7 will be set for read)
 * @param[out] data Pointer to store read data
 * @return SPI_OK on success, error code on failure
 */
spi_status_t spi_read_byte(uint8_t reg, uint8_t *data);

/**
 * @brief Read multiple bytes from SPI using burst mode
 *
 * @param reg Starting register address (7-bit, bit 7 will be set for read)
 * @param[out] buffer Buffer to store read data
 * @param len Number of bytes to read
 * @return SPI_OK on success, error code on failure
 */
spi_status_t spi_read_burst(uint8_t reg, uint8_t *buffer, uint8_t len);

#endif
