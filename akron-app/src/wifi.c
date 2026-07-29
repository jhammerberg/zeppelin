#include "wifi.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

LOG_MODULE_REGISTER(wifi, LOG_LEVEL_INF);

/* Events we listen for to determine connection outcome */
#define WIFI_MGMT_EVENTS \
    (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

/* Signalled by the event handler once a connect result is available */
static struct k_sem wifi_connected_sem;

/* Callback registration for Wi-Fi management events */
static struct net_mgmt_event_callback wifi_mgmt_cb;

/* Result of the most recent connection attempt (0 = success) */
static int wifi_connect_status = -1;

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback* cb,
                                    uint32_t mgmt_event, struct net_if* iface) {
    ARG_UNUSED(iface);

    switch (mgmt_event) {
        case (uint32_t)NET_EVENT_WIFI_CONNECT_RESULT: {
            const struct wifi_status* status = (const struct wifi_status*)cb->info;

            if (status->status) {
                LOG_ERR("Connection request failed (status %d)", status->status);
                wifi_connect_status = -1;
            } else {
                LOG_INF("Connected to Wi-Fi");
                wifi_connect_status = 0;
            }

            k_sem_give(&wifi_connected_sem);
            break;
        }

        case (uint32_t)NET_EVENT_WIFI_DISCONNECT_RESULT: {
            LOG_INF("Disconnected from Wi-Fi");
            wifi_connect_status = -1;
            k_sem_give(&wifi_connected_sem);
            break;
        }

        default:
            break;
    }
}

int wifi_connect(const char* ssid, const char* psk, const int timeout) {
    if (ssid == NULL) {
        LOG_ERR("SSID must not be NULL");
        return -EINVAL;
    }

    /* Grab the default network interface (the Wi-Fi device) */
    struct net_if* iface = net_if_get_first_wifi();
    if (iface == NULL) {
        LOG_ERR("No Wi-Fi interface available");
        return -ENODEV;
    }

    /* Prepare the semaphore used to wait for the connection result */
    k_sem_init(&wifi_connected_sem, 0, 1);
    wifi_connect_status = -1;

    /* Register for the Wi-Fi management events */
    net_mgmt_init_event_callback(&wifi_mgmt_cb, (void*)wifi_mgmt_event_handler,
                                 WIFI_MGMT_EVENTS);
    net_mgmt_add_event_callback(&wifi_mgmt_cb);

    /* Build the connection request parameters */
    struct wifi_connect_req_params params = {0};

    params.ssid = (const uint8_t*)ssid;
    params.ssid_length = strlen(ssid);
    params.channel = WIFI_CHANNEL_ANY;
    params.band = WIFI_FREQ_BAND_UNKNOWN;
    params.mfp = WIFI_MFP_OPTIONAL;

    if (psk != NULL && strlen(psk) > 0) {
        params.psk = (const uint8_t*)psk;
        params.psk_length = strlen(psk);
        params.security = WIFI_SECURITY_TYPE_PSK;
    } else {
        params.security = WIFI_SECURITY_TYPE_NONE;
    }

    LOG_INF("Connecting to SSID \"%s\"...", ssid);

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
    if (ret) {
        LOG_ERR("Wi-Fi connect request failed (%d)", ret);
        net_mgmt_del_event_callback(&wifi_mgmt_cb);
        return ret;
    }

    /* Wait for the connect result event, up to the requested timeout */
    ret = k_sem_take(&wifi_connected_sem, K_MSEC(timeout));

    net_mgmt_del_event_callback(&wifi_mgmt_cb);

    if (ret != 0) {
        LOG_ERR("Timed out waiting for Wi-Fi connection");
        return -ETIMEDOUT;
    }

    return wifi_connect_status;
}
