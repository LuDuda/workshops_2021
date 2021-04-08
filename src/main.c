/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>

#include <dk_buttons_and_leds.h>

#define STATUS_LED          DK_LED3
#define STATUS_LED_INTERVAL 500

LOG_MODULE_REGISTER(APP);

void init_leds(void)
{
	int err = dk_leds_init();
	if (err) {
		LOG_ERR("Failed to initialize LEDs module. Error %d\n", err);
	}
}

void main(void)
{
	int err;
	int cnt = 0;

	LOG_INF("Hello World!\n");
	LOG_DBG("Board: %s\n", CONFIG_BOARD);

	init_leds();

	while (1) {
		err = dk_set_led(STATUS_LED, cnt++%2);
		if (err) {
			LOG_ERR("Failed to set the LED. Error %d\n", err);
		}

		k_msleep(STATUS_LED_INTERVAL);
	}
}
