/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>

#include <drivers/pwm.h>
#include <device.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/nus.h>

#include <dk_buttons_and_leds.h>

#define STATUS_LED          DK_LED3
#define STATUS_LED_INTERVAL 500
#define APP_LED             DK_LED4

#define LED_THREAD_STACK_SIZE 500
#define LED_THREAD_PRIORITY   5

#define PWM_LEVEL_MAX (16-1)

#define LED_PWM_NODE DT_NODELABEL(pwm_led4)
#define LED_PWM_DEVICE DT_PWMS_LABEL(LED_PWM_NODE)
#define LED_PWM_CHANNEL DT_PWMS_CHANNEL(LED_PWM_NODE)

#define DEVICE_NAME "Workshop"

LOG_MODULE_REGISTER(APP);

static volatile int level = 0;

static struct bt_conn *bt_conn = NULL;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, sizeof(DEVICE_NAME) - 1),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

static void update_pwm(bool is_button_triggered)
{
	pwm_pin_set_usec(
		device_get_binding(LED_PWM_DEVICE),
		LED_PWM_CHANNEL,
		1000,
		1000 * level / PWM_LEVEL_MAX,
		0
	);

	if (is_button_triggered) {
		uint8_t data = '0' + level;

		int err = bt_nus_send(bt_conn, &data, sizeof(data));
		if (err) {
			LOG_ERR("Failed to send NUS notification. Error %d", err);
		}
	}
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	uint32_t button_mask = button_state & has_changed;

	if (button_mask & DK_BTN1_MSK)
	{
		LOG_INF("Button 1 has been pushed.");

		if (level > 0) {
			level--;
			update_pwm(true);
		}
	}

	if (button_mask & DK_BTN2_MSK)
	{
		LOG_INF("Button 2 has been pushed.");

		if (level < PWM_LEVEL_MAX) {
			level++;
			update_pwm(true);
		}
	}
}

static void init_leds(void)
{
	int err = dk_leds_init();
	if (err) {
		LOG_ERR("Failed to initialize LEDs module. Error %d\n", err);
	}
}

static void init_buttons(void)
{
	int err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize buttons module. Error %d\n", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_ERR("Bluetooth Low Energy: Connection failed. Error %d\n", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Bluetooth Low Energy: Connected to %s\n", addr);

	dk_set_led_on(STATUS_LED);

	bt_conn = conn;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Bluetooth Low Energy: Disconnected. Reason %d\n", reason);

	dk_set_led_off(STATUS_LED);

	bt_conn = NULL;
}

static struct bt_conn_cb conn_callbacks = {
	.connected    = connected,
	.disconnected = disconnected,
};

static void on_nus_receive(struct bt_conn *conn, const uint8_t *const data,
						   uint16_t len)
{
	char addr[BT_ADDR_LE_STR_LEN] = {0};
	char command;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

	LOG_INF("Received data from: %s, length: %d.", addr, len);

	if (len) {
		command = data[0];

		switch (command) {
		case '-':
			if (level > 0) {
				level--;
				update_pwm(false);
			}
		break;
		case '+':
			if (level < PWM_LEVEL_MAX) {
				level++;
				update_pwm(false);
			}
		break;
		default:
			LOG_ERR("Unknown character received - %c.", command);
			break;
		}
	}
}

static struct bt_nus_cb nus_cb = {
	.received = on_nus_receive,
};

static void on_ble_init(int err)
{
	if (err) {
		LOG_ERR("Failed to initialize BLE module. Error %d\n", err);
		return;
	}

	err = bt_nus_init(&nus_cb);
	if (err) {
		printk("Failed to initialize BLE NUS service. Error %d)", err);
	}
}

static void init_ble(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(on_ble_init);
	if (err) {
		LOG_ERR("Failed to initialize BLE module. Error %d\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Failed to start advertising. Error %d\n", err);
	}
}

static void led_thread(void)
{
	int cnt = 0;

	while (1) {
		int err = dk_set_led(STATUS_LED, cnt++%2);
		if (err) {
			LOG_ERR("Failed to set the LED. Error %d\n", err);
		}

		k_msleep(STATUS_LED_INTERVAL);
	}
}

void main(void)
{
	LOG_INF("Hello World!\n");
	LOG_DBG("Board: %s\n", CONFIG_BOARD);

	init_leds();
	init_buttons();
	init_ble();
}

K_THREAD_DEFINE(led_tid, LED_THREAD_STACK_SIZE,
                (k_thread_entry_t)led_thread, NULL, NULL, NULL,
                LED_THREAD_PRIORITY, 0, 0);
