#include "app/fsm.h"
#include "config.h"
#include "drivers/lsm6dso.h"
#include "hal/clock.h"
#include "hal/gpio.h"
#include "hal/spi.h"
#include "hal/timer.h"
#include "hal/uart.h"
#include "hal/watchdog.h"
#include "stm32f10x.h"

static void error_halt(const char *msg) {
	uart_puts(msg);
	while (1) {
		__NOP();
	}
}

int main(void) {
	fsm_init();

	clock_status_t clk_status = clock_init();
	gpio_init();
	spi_init();
	uart_init();
	timer_init();

	uart_puts("FW: " FW_VERSION_STRING "\r\n");

	if (clk_status == CLOCK_ERR_HSE_TIMEOUT) {
		error_halt("ERR: HSE startup timeout\r\n");
	} else if (clk_status == CLOCK_ERR_PLL_TIMEOUT) {
		error_halt("ERR: PLL lock timeout\r\n");
	}

	watchdog_init();
	uart_debug_printf("Watchdog initialized, timeout=%ums\r\n", IWDG_TIMEOUT_MS);

	for (;;) {
		fsm_run();
		watchdog_refresh();
	}

	return 0;
}
