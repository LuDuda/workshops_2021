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

static void led_timer_handler(struct k_timer *timer_id);
K_TIMER_DEFINE(led_timer, led_timer_handler, NULL);

static void init_leds(void)
{
	int err = dk_leds_init();
	if (err) {
		LOG_ERR("Failed to initialize LEDs module. Error %d\n", err);
	}
}

static void led_timer_handler(struct k_timer *timer_id)
{
	static int cnt = 0;

	int err = dk_set_led(STATUS_LED, cnt++%2);
	if (err) {
		LOG_ERR("Failed to set the LED. Error %d\n", err);
	}
}

void main(void)
{
	LOG_INF("Hello World!\n");
	LOG_DBG("Board: %s\n", CONFIG_BOARD);

	init_leds();

	// Start status LED timer.
	k_timer_start(&led_timer, K_MSEC(STATUS_LED_INTERVAL), K_MSEC(STATUS_LED_INTERVAL));
}
