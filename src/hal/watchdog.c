#include "hal/watchdog.h"
#include "config.h"
#include "stm32f10x.h"

static bool g_watchdog_initialized = false;

void watchdog_init(void) {
	if (g_watchdog_initialized) {
		return;
	}

	IWDG->KR = 0x5555U;

	IWDG->PR = 0x04U;

	IWDG->RLR = (IWDG_RELOAD > 0xFFFU) ? 0xFFFU : IWDG_RELOAD;

	IWDG->KR = 0xCCCCU;

	g_watchdog_initialized = true;
}

void watchdog_refresh(void) { IWDG->KR = 0xAAAAU; }

bool watchdog_is_enabled(void) { return g_watchdog_initialized; }
