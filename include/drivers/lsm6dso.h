/**
 * @file lsm6dso.h
 * @brief LSM6DSO 6-axis IMU driver
 * @author STM32_LSM6DSO Project
 *
 * This module provides communication with the LSM6DSO accelerometer/gyroscope
 * sensor via SPI interface.
 */

#ifndef DRIVERS_LSM6DSO_H
#define DRIVERS_LSM6DSO_H

#include <stdbool.h>
#include <stdint.h>

/*============================================================================
 * Register Addresses
 *===========================================================================*/
#define LSM6DSO_WHO_AM_I   (0x0FU) /**< Device identification register */
#define LSM6DSO_CTRL1_XL   (0x10U) /**< Accelerometer control register */
#define LSM6DSO_CTRL2_G    (0x11U) /**< Gyroscope control register */
#define LSM6DSO_CTRL3_C    (0x12U) /**< Control register 3 */
#define LSM6DSO_CTRL4_C    (0x13U) /**< Control register 4 */
#define LSM6DSO_CTRL5_C    (0x14U) /**< Control register 5 */
#define LSM6DSO_CTRL6_C    (0x15U) /**< Control register 6 */
#define LSM6DSO_CTRL7_G    (0x16U) /**< Gyroscope control register 7 */
#define LSM6DSO_CTRL8_XL   (0x17U) /**< Accelerometer control register 8 */
#define LSM6DSO_CTRL9_XL   (0x18U) /**< Accelerometer control register 9 */
#define LSM6DSO_CTRL10_C   (0x19U) /**< Control register 10 */
#define LSM6DSO_STATUS_REG (0x1EU) /**< Status register */
#define LSM6DSO_OUT_TEMP_L (0x20U) /**< Temperature data low */
#define LSM6DSO_OUT_TEMP_H (0x21U) /**< Temperature data high */
#define LSM6DSO_OUTX_L_G   (0x22U) /**< Gyroscope X-axis low */
#define LSM6DSO_OUTX_H_G   (0x23U) /**< Gyroscope X-axis high */
#define LSM6DSO_OUTY_L_G   (0x24U) /**< Gyroscope Y-axis low */
#define LSM6DSO_OUTY_H_G   (0x25U) /**< Gyroscope Y-axis high */
#define LSM6DSO_OUTZ_L_G   (0x26U) /**< Gyroscope Z-axis low */
#define LSM6DSO_OUTZ_H_G   (0x27U) /**< Gyroscope Z-axis high */
#define LSM6DSO_OUTX_L_XL  (0x28U) /**< Accelerometer X-axis low */
#define LSM6DSO_OUTX_H_XL  (0x29U) /**< Accelerometer X-axis high */
#define LSM6DSO_OUTY_L_XL  (0x2AU) /**< Accelerometer Y-axis low */
#define LSM6DSO_OUTY_H_XL  (0x2BU) /**< Accelerometer Y-axis high */
#define LSM6DSO_OUTZ_L_XL  (0x2CU) /**< Accelerometer Z-axis low */
#define LSM6DSO_OUTZ_H_XL  (0x2DU) /**< Accelerometer Z-axis high */

/*============================================================================
 * Configuration Values
 *===========================================================================*/
#define LSM6DSO_EXPECTED_ID        (0x6CU) /**< Expected WHO_AM_I value */
#define LSM6DSO_GYRO_208HZ_2000DPS (0x5CU) /**< Gyro: 208Hz, ±2000dps */
#define LSM6DSO_BDU_ENABLE         (0x40U) /**< Block Data Update enable */
#define LSM6DSO_ACC_208HZ_2G       (0x50U) /**< Accel: 208Hz, ±2g */

/*============================================================================
 * Status Register Bits
 *===========================================================================*/
#define LSM6DSO_STATUS_XLDA (0x01U) /**< Accelerometer data available */
#define LSM6DSO_STATUS_GDA  (0x02U) /**< Gyroscope data available */
#define LSM6DSO_STATUS_TDA  (0x04U) /**< Temperature data available */

/*============================================================================
 * Sensitivity Constants
 *===========================================================================*/
#define LSM6DSO_GYRO_DPSx100_PER_LSB                                           \
	(7) /**< Gyro: 70 mdps/LSB = 0.07 dps/LSB */
#define LSM6DSO_ACC_mg_X1000_PER_LSB (61) /**< Accel: 0.061 mg/LSB */

/*============================================================================
 * Timing Constants
 *===========================================================================*/
#define LSM6DSO_STABILIZATION_MS (100U) /**< Sensor stabilization time */
#define LSM6DSO_RETRY_DELAY_MS   (10U)  /**< Delay between ID check retries */
#define LSM6DSO_MAX_RETRIES      (3U)   /**< Maximum ID check retries */

/**
 * @brief Sensor data structure
 */
typedef struct {
	int16_t gyro_x;  /**< Gyroscope X-axis raw data */
	int16_t gyro_y;  /**< Gyroscope Y-axis raw data */
	int16_t gyro_z;  /**< Gyroscope Z-axis raw data */
	int16_t accel_x; /**< Accelerometer X-axis raw data */
	int16_t accel_y; /**< Accelerometer Y-axis raw data */
	int16_t accel_z; /**< Accelerometer Z-axis raw data */
} lsm6dso_data_t;

/**
 * @brief LSM6DSO operation status codes
 */
typedef enum {
	LSM6DSO_OK = 0,          /**< Operation successful */
	LSM6DSO_ERR_SPI = -1,    /**< SPI communication error */
	LSM6DSO_ERR_ID = -2,     /**< Invalid device ID */
	LSM6DSO_ERR_TIMEOUT = -3 /**< Operation timed out */
} lsm6dso_status_t;

/**
 * @brief Initialize LSM6DSO sensor
 *
 * Configures:
 * - Gyroscope: 208Hz, ±2000dps
 * - Accelerometer: 208Hz, ±2g
 * - Block Data Update enabled
 *
 * @return LSM6DSO_OK on success, error code on failure
 */
lsm6dso_status_t lsm6dso_init(void);

/**
 * @brief Read device identification register
 *
 * @param[out] id Pointer to store device ID
 * @return LSM6DSO_OK on success, error code on failure
 */
lsm6dso_status_t lsm6dso_read_id(uint8_t *id);

/**
 * @brief Read all sensor data
 *
 * Reads gyroscope and accelerometer data using burst read.
 *
 * @param[out] data Pointer to store sensor data
 * @return LSM6DSO_OK on success, error code on failure
 */
lsm6dso_status_t lsm6dso_read_data(lsm6dso_data_t *data);

/**
 * @brief Check if new data is available
 *
 * @return true if gyroscope or accelerometer data is available
 */
bool lsm6dso_data_ready(void);

#endif
