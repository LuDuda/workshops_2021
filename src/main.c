/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(APP);

void main(void)
{
	LOG_INF("Hello World!\n");
	LOG_DBG("Board: %s\n", CONFIG_BOARD);
}
