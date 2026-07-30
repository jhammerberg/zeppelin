/*
 * All initialization
 */

#pragma once

#include <helium/time.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>

extern const struct gpio_dt_spec led;

// Initialize all GPIO, start networking, sync system time, etc
int init(void);
