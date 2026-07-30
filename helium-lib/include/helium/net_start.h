/* Establishes the network connection either via Wi-Fi or Ethernet.
 * native_sim and esp32 compatible
 */

#pragma once

#include <zephyr/types.h>

struct wifi_config {
    const char* ssid;
    const char* password;
    const int timeout;  // Connection timeout in milliseconds
};

struct static_ip_config {
    const char* ip;
    const char* netmask;
    const char* gateway;
};

struct network_config {
    const int prefer_dhcp;  // bool; fall back to static_ip if DHCP fails
    const void* wifi;       // can be null or a struct wifi_config*
    const void* static_ip;  // can be null or a struct static_ip_config*
};

/* Starts the network connection and configures IPv4 addressing.
 *
 * Returns 0 when the selected interface is ready, or a negative errno value.
 */
int net_start(const struct network_config* config);
