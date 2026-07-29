/*
 * Basic echo UDP server that runs on a dedicated thread.
 */

#pragma once

#include <zephyr/kernel.h>

// Spawn the dedicated UDP echo server thread.
// Returns the thread id of the spawned server thread.
k_tid_t udp_echo_server_start(void);
