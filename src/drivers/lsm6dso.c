#include "drivers/lsm6dso.h"
#include "hal/spi.h"
#include "hal/timer.h"
#include <string.h>

#define BURST_READ_LEN (6U)

static bool g_lsm6dso_initialized = false;

static s16 combine_bytes(u8 lsb, u8 msb) {
	return (s16)(((s16)msb << 8) | (s16)lsb);
}

lsm6dso_status_t lsm6dso_init(void) {
	if (g_lsm6dso_initialized) {
		return LSM6DSO_OK;
	}

	spi_status_t status;

	status = spi_write_byte(LSM6DSO_CTRL2_G, LSM6DSO_GYRO_208HZ_2000DPS);
	if (status != SPI_OK) {
		return LSM6DSO_ERR_SPI;
	}

	status = spi_write_byte(LSM6DSO_CTRL1_XL, LSM6DSO_ACC_208HZ_2G);
	if (status != SPI_OK) {
		return LSM6DSO_ERR_SPI;
	}

	status = spi_write_byte(LSM6DSO_CTRL8_XL, 0x00U);
	if (status != SPI_OK) {
		return LSM6DSO_ERR_SPI;
	}

	status = spi_write_byte(LSM6DSO_CTRL3_C, (LSM6DSO_BDU_ENABLE | 0x04U));
	if (status != SPI_OK) {
		return LSM6DSO_ERR_SPI;
	}

	g_lsm6dso_initialized = true;
	return LSM6DSO_OK;
}

lsm6dso_status_t lsm6dso_read_id(u8 *id) {
	if (id == NULL) {
		return LSM6DSO_ERR_SPI;
	}

	spi_status_t status = spi_read_byte(LSM6DSO_WHO_AM_I, id);
	return (status == SPI_OK) ? LSM6DSO_OK : LSM6DSO_ERR_SPI;
}

bool lsm6dso_data_ready(void) {
	u8 status = 0U;
	spi_read_byte(LSM6DSO_STATUS_REG, &status);
	return ((status & (LSM6DSO_STATUS_GDA | LSM6DSO_STATUS_XLDA)) != 0U);
}

lsm6dso_status_t lsm6dso_read_data(lsm6dso_data_t *data) {
	if (data == NULL) {
		return LSM6DSO_ERR_SPI;
	}

	u8 gyro_buf[BURST_READ_LEN];
	u8 accel_buf[BURST_READ_LEN];

	spi_status_t status =
	    spi_read_burst(LSM6DSO_OUTX_L_G, gyro_buf, BURST_READ_LEN);
	if (status != SPI_OK) {
		return LSM6DSO_ERR_SPI;
	}

	status = spi_read_burst(LSM6DSO_OUTX_L_XL, accel_buf, BURST_READ_LEN);
	if (status != SPI_OK) {
		return LSM6DSO_ERR_SPI;
	}

	data->gyro_x = combine_bytes(gyro_buf[0], gyro_buf[1]);
	data->gyro_y = combine_bytes(gyro_buf[2], gyro_buf[3]);
	data->gyro_z = combine_bytes(gyro_buf[4], gyro_buf[5]);

	data->accel_x = combine_bytes(accel_buf[0], accel_buf[1]);
	data->accel_y = combine_bytes(accel_buf[2], accel_buf[3]);
	data->accel_z = combine_bytes(accel_buf[4], accel_buf[5]);

	return LSM6DSO_OK;
}
