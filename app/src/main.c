#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "setup.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define MAIN_LOOP_PERIOD_MS 300

int main(void)
{
	setup_init();

	while (1) {
		k_msleep(MAIN_LOOP_PERIOD_MS);
		setup_wdt_feed();
	}

	return 0;
}
