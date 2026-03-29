#include "app/fsm.h"
#include "config.h"
#include "drivers/lsm6dso.h"
#include "hal/timer.h"
#include "hal/uart.h"
#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

#define FORMAT_BUFFER_SIZE (128U)
#define OUTPUT_FORMAT_CSV  (1U)

static fsm_context_t g_fsm;
static char g_format_buffer[FORMAT_BUFFER_SIZE];
static bool g_header_printed = false;
static bool g_fsm_initialized = false;

static void format_signed_value(s32 value, u32 divisor, char *sign,
                                u32 *int_part, u32 *frac_part) {
	*sign = (value < 0) ? '-' : '+';
	u32 abs_val = (u32)((value < 0) ? -value : value);
	*int_part = abs_val / divisor;
	*frac_part = abs_val % divisor;
}

static bool format_imu_data(const lsm6dso_data_t *data) {
	s32 gx100 = (s32)data->gyro_x * LSM6DSO_GYRO_DPSx100_PER_LSB;
	s32 gy100 = (s32)data->gyro_y * LSM6DSO_GYRO_DPSx100_PER_LSB;
	s32 gz100 = (s32)data->gyro_z * LSM6DSO_GYRO_DPSx100_PER_LSB;

	s32 ax_ug = (s32)data->accel_x * LSM6DSO_ACC_mg_X1000_PER_LSB;
	s32 ay_ug = (s32)data->accel_y * LSM6DSO_ACC_mg_X1000_PER_LSB;
	s32 az_ug = (s32)data->accel_z * LSM6DSO_ACC_mg_X1000_PER_LSB;

	s32 ax_gx1000 =
	    (ax_ug >= 0) ? ((ax_ug + 500) / 1000) : ((ax_ug - 500) / 1000);
	s32 ay_gx1000 =
	    (ay_ug >= 0) ? ((ay_ug + 500) / 1000) : ((ay_ug - 500) / 1000);
	s32 az_gx1000 =
	    (az_ug >= 0) ? ((az_ug + 500) / 1000) : ((az_ug - 500) / 1000);

	char sgx, sgy, sgz, sax, say, saz;
	u32 gxi, gxf, gyi, gyf, gzi, gzf;
	u32 axi, axf, ayi, ayf, azi, azf;

	format_signed_value(gx100, 100U, &sgx, &gxi, &gxf);
	format_signed_value(gy100, 100U, &sgy, &gyi, &gyf);
	format_signed_value(gz100, 100U, &sgz, &gzi, &gzf);
	format_signed_value(ax_gx1000, 1000U, &sax, &axi, &axf);
	format_signed_value(ay_gx1000, 1000U, &say, &ayi, &ayf);
	format_signed_value(az_gx1000, 1000U, &saz, &azi, &azf);

	int result = snprintf(
	    g_format_buffer, sizeof(g_format_buffer),
	    "%c%lu.%02lu,%c%lu.%02lu,%c%lu.%02lu,%c%lu.%03lu,%c%lu.%03lu,%c%lu.%"
	    "03lu\r\n",
	    sgx, (unsigned long)gxi, (unsigned long)gxf, sgy, (unsigned long)gyi,
	    (unsigned long)gyf, sgz, (unsigned long)gzi, (unsigned long)gzf, sax,
	    (unsigned long)axi, (unsigned long)axf, say, (unsigned long)ayi,
	    (unsigned long)ayf, saz, (unsigned long)azi, (unsigned long)azf);

	return (result > 0) && ((size_t)result < sizeof(g_format_buffer));
}

static void print_csv_header(void) {
	if (!g_header_printed) {
		int result = snprintf(g_format_buffer, sizeof(g_format_buffer),
		                      "gx_dps,gy_dps,gz_dps,ax_g,ay_g,az_g\r\n");
		if ((result > 0) && ((size_t)result < sizeof(g_format_buffer))) {
			uart_puts(g_format_buffer);
		}
		g_header_printed = true;
	}
}

void fsm_init(void) {
	if (g_fsm_initialized) {
		return;
	}

	g_fsm.current_state = FSM_STATE_INIT;
	g_fsm.retry_count = LSM6DSO_MAX_RETRIES;
	g_fsm.stabilization_start = 0U;
	g_fsm.last_retry_time = 0U;
	g_header_printed = false;
	g_fsm_initialized = true;
}

void fsm_set_state(fsm_state_t new_state) {
	__disable_irq();
	g_fsm.current_state = new_state;
	__enable_irq();
}

fsm_state_t fsm_get_state(void) {
	fsm_state_t state;
	__disable_irq();
	state = g_fsm.current_state;
	__enable_irq();
	return state;
}

static void handle_init_state(void) {
	g_fsm.retry_count = LSM6DSO_MAX_RETRIES;
	g_fsm.stabilization_start = get_system_tick();
	g_fsm.last_retry_time = get_system_tick();

	lsm6dso_status_t status = lsm6dso_init();
	if (status != LSM6DSO_OK) {
		fsm_set_state(FSM_STATE_ERROR);
		return;
	}
	fsm_set_state(FSM_STATE_STABILIZING);
}

static void handle_stabilizing_state(void) {
	u32 current_tick = get_system_tick();

	if ((current_tick - g_fsm.stabilization_start) < LSM6DSO_STABILIZATION_MS) {
		return;
	}

	if ((g_fsm.retry_count > 0U) &&
	    ((current_tick - g_fsm.last_retry_time) >= LSM6DSO_RETRY_DELAY_MS)) {

		u8 chip_id = 0U;
		lsm6dso_status_t status = lsm6dso_read_id(&chip_id);

		if (status != LSM6DSO_OK) {
			fsm_set_state(FSM_STATE_ERROR);
			return;
		}

		if (chip_id == LSM6DSO_EXPECTED_ID) {
			fsm_set_state(FSM_STATE_IDLE);
		} else {
			g_fsm.retry_count--;
			g_fsm.last_retry_time = current_tick;

			int result =
			    snprintf(g_format_buffer, sizeof(g_format_buffer),
			             "ERR:ID 0x%02X, retries left: %u\r\n", (unsigned int)chip_id,
			             (unsigned int)g_fsm.retry_count);

			if ((result > 0) && ((size_t)result < sizeof(g_format_buffer))) {
				uart_puts(g_format_buffer);
			}

			if (g_fsm.retry_count == 0U) {
				fsm_set_state(FSM_STATE_ERROR);
			}
		}
	}
}

static void handle_read_state(void) {
	if (!lsm6dso_data_ready()) {
		fsm_set_state(FSM_STATE_IDLE);
		return;
	}

	lsm6dso_status_t status = lsm6dso_read_data(&g_fsm.sensor_data);
	if (status != LSM6DSO_OK) {
		fsm_set_state(FSM_STATE_ERROR);
	} else {
		fsm_set_state(FSM_STATE_PROCESS);
	}
}

static void handle_process_state(void) {
#if OUTPUT_FORMAT_CSV
	print_csv_header();
#endif
	if (format_imu_data(&g_fsm.sensor_data)) {
		fsm_set_state(FSM_STATE_OUTPUT);
	} else {
		fsm_set_state(FSM_STATE_ERROR);
	}
}

static void handle_output_state(void) {
	uart_puts(g_format_buffer);
	fsm_set_state(FSM_STATE_IDLE);
}

static void handle_error_state(void) {
	uart_puts("System halted due to error\r\n");
	while (1) {
		__NOP();
	}
}

void fsm_run(void) {
	switch (g_fsm.current_state) {
	case FSM_STATE_INIT:
		handle_init_state();
		break;
	case FSM_STATE_STABILIZING:
		handle_stabilizing_state();
		break;
	case FSM_STATE_READ:
		handle_read_state();
		break;
	case FSM_STATE_PROCESS:
		handle_process_state();
		break;
	case FSM_STATE_OUTPUT:
		handle_output_state();
		break;
	case FSM_STATE_IDLE:
		__WFI();
		break;
	case FSM_STATE_ERROR:
		handle_error_state();
		break;
	default:
		fsm_set_state(FSM_STATE_ERROR);
		break;
	}
}
