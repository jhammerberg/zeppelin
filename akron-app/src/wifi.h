/*
 * Standard Wi-Fi boilerplate for ESP32
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

int wifi_connect(const char* ssid, const char* psk, const int timeout);
