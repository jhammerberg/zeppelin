#include "helium/net_start.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#if defined(CONFIG_NET_DHCPV4)
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/net_mgmt.h>
#endif

#if defined(CONFIG_NET_L2_ETHERNET)
#include <zephyr/net/ethernet.h>
#endif

#if defined(CONFIG_WIFI) && defined(CONFIG_NET_L2_WIFI_MGMT)
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#endif

LOG_MODULE_REGISTER(net_start, LOG_LEVEL_INF);

#define DHCP_TIMEOUT_MS 30000

K_MUTEX_DEFINE(net_start_mutex);

static int validate_config(const struct network_config* config) {
    if (config == NULL) {
        LOG_ERR("Network config must not be null");
        return -EINVAL;
    }

    if (config->static_ip == NULL && config->wifi == NULL) {
        LOG_ERR("Both static IP and Wi-Fi configs cannot be null");
        return -EINVAL;
    }

    if (config->static_ip != NULL) {
        const struct static_ip_config* static_ip = config->static_ip;

        if (static_ip->ip == NULL || static_ip->netmask == NULL ||
            static_ip->gateway == NULL) {
            LOG_ERR("Static IP, netmask, and gateway must not be null");
            return -EINVAL;
        }
    }

    if (config->wifi != NULL) {
        const struct wifi_config* wifi = config->wifi;

        if (wifi->ssid == NULL || wifi->ssid[0] == '\0') {
            LOG_ERR("Wi-Fi SSID must not be empty");
            return -EINVAL;
        }

        if (wifi->timeout <= 0) {
            LOG_ERR("Wi-Fi timeout must be greater than zero");
            return -EINVAL;
        }
    }

#if !defined(CONFIG_WIFI) || !defined(CONFIG_NET_L2_WIFI_MGMT)
    if (config->static_ip == NULL) {
        LOG_ERR("Wi-Fi is disabled and no static IP config was supplied");
        return -ENOTSUP;
    }
#endif

#if !defined(CONFIG_NET_IPV4)
    LOG_ERR("IPv4 networking is disabled");
    return -ENOTSUP;
#endif

#if !defined(CONFIG_NET_L2_ETHERNET)
    if (config->wifi == NULL) {
        LOG_ERR("Ethernet is disabled and no Wi-Fi config was supplied");
        return -ENOTSUP;
    }
#endif

    return 0;
}

#if defined(CONFIG_NET_L2_ETHERNET)
static void find_ethernet_iface(struct net_if* iface, void* user_data) {
    struct net_if** result = user_data;

    if (*result == NULL && net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET) &&
        !net_if_is_wifi(iface)) {
        *result = iface;
    }
}

static struct net_if* get_ethernet_iface(void) {
    struct net_if* iface = NULL;

    net_if_foreach(find_ethernet_iface, &iface);
    return iface;
}
#endif

static int bring_iface_up(struct net_if* iface) {
    if (net_if_is_up(iface)) {
        return 0;
    }

    int ret = net_if_up(iface);
    if (ret < 0 && ret != -EALREADY) {
        LOG_ERR("Failed to bring network interface up: %d", ret);
        return ret;
    }

    return 0;
}

#if defined(CONFIG_WIFI) && defined(CONFIG_NET_L2_WIFI_MGMT)
K_SEM_DEFINE(wifi_connect_sem, 0, 1);

static struct net_mgmt_event_callback wifi_mgmt_cb;
static struct net_if* wifi_connect_iface;
static int wifi_connect_status;

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback* cb,
                                    uint64_t mgmt_event, struct net_if* iface) {
    if (iface != wifi_connect_iface) {
        return;
    }

    const struct wifi_status* status = cb->info;

    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        if (status == NULL || status->status != 0) {
            wifi_connect_status = -EIO;
            LOG_ERR("Wi-Fi connection failed (status %d)",
                    status == NULL ? -1 : status->status);
        } else {
            wifi_connect_status = 0;
        }
    } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        wifi_connect_status = -ECONNREFUSED;
        LOG_ERR("Wi-Fi disconnected while connecting (status %d)",
                status == NULL ? -1 : status->status);
    } else {
        return;
    }

    k_sem_give(&wifi_connect_sem);
}

static int connect_wifi(struct net_if* iface, const struct wifi_config* wifi) {
    size_t ssid_length = strlen(wifi->ssid);
    size_t password_length = wifi->password == NULL ? 0 : strlen(wifi->password);

    if (ssid_length > WIFI_SSID_MAX_LEN) {
        LOG_ERR("Wi-Fi SSID is too long");
        return -EINVAL;
    }

    if (password_length != 0 &&
        (password_length < WIFI_PSK_MIN_LEN || password_length > WIFI_PSK_MAX_LEN)) {
        LOG_ERR("Wi-Fi password must be between %d and %d characters", WIFI_PSK_MIN_LEN,
                WIFI_PSK_MAX_LEN);
        return -EINVAL;
    }

    struct wifi_connect_req_params params = {
        .ssid = (const uint8_t*)wifi->ssid,
        .ssid_length = (uint8_t)ssid_length,
        .psk = (const uint8_t*)wifi->password,
        .psk_length = (uint8_t)password_length,
        .band = WIFI_FREQ_BAND_UNKNOWN,
        .channel = WIFI_CHANNEL_ANY,
        .security =
            password_length == 0 ? WIFI_SECURITY_TYPE_NONE : WIFI_SECURITY_TYPE_PSK,
        .mfp = WIFI_MFP_OPTIONAL,
        .timeout = DIV_ROUND_UP(wifi->timeout, MSEC_PER_SEC),
    };

    k_sem_reset(&wifi_connect_sem);
    wifi_connect_iface = iface;
    wifi_connect_status = -EIO;

    net_mgmt_init_event_callback(
        &wifi_mgmt_cb, wifi_mgmt_event_handler,
        NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_mgmt_cb);

    LOG_INF("Connecting to Wi-Fi SSID \"%s\"", wifi->ssid);

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
    if (ret < 0) {
        LOG_ERR("Wi-Fi connect request failed: %d", ret);
        goto out;
    }

    int64_t deadline = k_uptime_get() + wifi->timeout;

    do {
        int64_t remaining = deadline - k_uptime_get();
        int wait_ms = (int)MIN(remaining, 100);

        if (k_sem_take(&wifi_connect_sem, K_MSEC(wait_ms)) == 0) {
            ret = wifi_connect_status;
            goto connected;
        }

        struct wifi_iface_status status = {0};
        ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(status));
        if (ret == 0 && status.state == WIFI_STATE_COMPLETED) {
            ret = 0;
            goto connected;
        }
    } while (k_uptime_get() < deadline);

    LOG_ERR("Timed out waiting for Wi-Fi connection");
    ret = -ETIMEDOUT;
    (void)net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
    goto out;

connected:
    if (ret == 0) {
        LOG_INF("Connected to Wi-Fi");
    }

out:
    net_mgmt_del_event_callback(&wifi_mgmt_cb);
    wifi_connect_iface = NULL;
    return ret;
}
#endif

struct ipv4_address_list {
    struct in_addr addresses[MAX(NET_IF_MAX_IPV4_ADDR, 1)];
    size_t count;
};

static void collect_manual_ipv4_address(struct net_if* iface,
                                        struct net_if_addr* address, void* user_data) {
    ARG_UNUSED(iface);

    struct ipv4_address_list* list = user_data;

    if (address->addr_type == NET_ADDR_MANUAL &&
        list->count < ARRAY_SIZE(list->addresses)) {
        list->addresses[list->count++] = address->address.in_addr;
    }
}

static void remove_manual_ipv4_addresses(struct net_if* iface) {
    struct ipv4_address_list list = {0};

    net_if_ipv4_addr_foreach(iface, collect_manual_ipv4_address, &list);

    for (size_t i = 0; i < list.count; ++i) {
        (void)net_if_ipv4_addr_rm(iface, &list.addresses[i]);
    }
}

static int configure_static_ipv4(struct net_if* iface,
                                 const struct static_ip_config* config) {
    struct in_addr address;
    struct in_addr netmask;
    struct in_addr gateway;

    int ret = net_addr_pton(AF_INET, config->ip, &address);
    if (ret < 0) {
        LOG_ERR("Invalid static IPv4 address: %s", config->ip);
        return -EINVAL;
    }

    ret = net_addr_pton(AF_INET, config->netmask, &netmask);
    if (ret < 0) {
        LOG_ERR("Invalid IPv4 netmask: %s", config->netmask);
        return -EINVAL;
    }

    ret = net_addr_pton(AF_INET, config->gateway, &gateway);
    if (ret < 0) {
        LOG_ERR("Invalid IPv4 gateway: %s", config->gateway);
        return -EINVAL;
    }

#if defined(CONFIG_NET_DHCPV4)
    net_dhcpv4_stop(iface);
#endif
    remove_manual_ipv4_addresses(iface);

    if (net_if_ipv4_addr_add(iface, &address, NET_ADDR_MANUAL, 0) == NULL) {
        LOG_ERR("Failed to add static IPv4 address");
        return -ENOSPC;
    }

    if (!net_if_ipv4_set_netmask_by_addr(iface, &address, &netmask)) {
        LOG_ERR("Failed to set IPv4 netmask");
        (void)net_if_ipv4_addr_rm(iface, &address);
        return -EINVAL;
    }

    net_if_ipv4_set_gw(iface, &gateway);
    LOG_INF("Configured static IPv4 address %s", config->ip);
    return 0;
}

#if defined(CONFIG_NET_DHCPV4)
K_SEM_DEFINE(dhcp_bound_sem, 0, 1);

static struct net_mgmt_event_callback dhcp_mgmt_cb;
static struct net_if* dhcp_iface;
static bool dhcp_address_found;

static void find_dhcp_ipv4_address(struct net_if* iface, struct net_if_addr* address,
                                   void* user_data) {
    ARG_UNUSED(iface);
    ARG_UNUSED(user_data);

    if (address->addr_type == NET_ADDR_DHCP) {
        dhcp_address_found = true;
    }
}

static bool has_dhcp_ipv4_address(struct net_if* iface) {
    dhcp_address_found = false;
    net_if_ipv4_addr_foreach(iface, find_dhcp_ipv4_address, NULL);
    return dhcp_address_found;
}

static void dhcp_mgmt_event_handler(struct net_mgmt_event_callback* cb,
                                    uint64_t mgmt_event, struct net_if* iface) {
    ARG_UNUSED(cb);

    if (mgmt_event == NET_EVENT_IPV4_DHCP_BOUND && iface == dhcp_iface) {
        k_sem_give(&dhcp_bound_sem);
    }
}

static int configure_dhcp(struct net_if* iface, int timeout_ms) {
    if (has_dhcp_ipv4_address(iface)) {
        LOG_INF("Network interface already has a DHCP lease");
        return 0;
    }

    remove_manual_ipv4_addresses(iface);
    k_sem_reset(&dhcp_bound_sem);
    dhcp_iface = iface;

    net_mgmt_init_event_callback(&dhcp_mgmt_cb, dhcp_mgmt_event_handler,
                                 NET_EVENT_IPV4_DHCP_BOUND);
    net_mgmt_add_event_callback(&dhcp_mgmt_cb);

    LOG_INF("Requesting an IPv4 address via DHCP");
    net_dhcpv4_start(iface);

    int ret = k_sem_take(&dhcp_bound_sem, K_MSEC(timeout_ms));
    if (ret < 0 && !has_dhcp_ipv4_address(iface)) {
        LOG_ERR("Timed out waiting for a DHCP lease");
        net_dhcpv4_stop(iface);
        ret = -ETIMEDOUT;
    } else {
        LOG_INF("DHCP lease acquired");
        ret = 0;
    }

    net_mgmt_del_event_callback(&dhcp_mgmt_cb);
    dhcp_iface = NULL;
    return ret;
}
#endif

static int configure_ipv4(struct net_if* iface, const struct network_config* config) {
    const struct static_ip_config* static_ip = config->static_ip;

#if defined(CONFIG_NET_DHCPV4)
    if (config->prefer_dhcp || static_ip == NULL) {
        const struct wifi_config* wifi = config->wifi;
        int timeout_ms = wifi == NULL ? DHCP_TIMEOUT_MS : wifi->timeout;
        int ret = configure_dhcp(iface, timeout_ms);

        if (ret == 0 || static_ip == NULL) {
            return ret;
        }

        LOG_WRN("DHCP failed; falling back to static IPv4 configuration");
    }
#else
    if (config->prefer_dhcp) {
        LOG_WRN("DHCP is not enabled; using static IPv4 configuration");
    }
#endif

    if (static_ip == NULL) {
        LOG_ERR("No supported IPv4 configuration was supplied");
        return -ENOTSUP;
    }

    return configure_static_ipv4(iface, static_ip);
}

int net_start(const struct network_config* config) {
    int ret = validate_config(config);
    if (ret < 0) {
        return ret;
    }

    k_mutex_lock(&net_start_mutex, K_FOREVER);

    struct net_if* iface = NULL;

#if defined(CONFIG_WIFI) && defined(CONFIG_NET_L2_WIFI_MGMT)
    if (config->wifi != NULL) {
        iface = net_if_get_first_wifi();
        if (iface == NULL) {
            LOG_ERR("No Wi-Fi network interface is available");
            ret = -ENODEV;
            goto out;
        }
    }
#endif

#if defined(CONFIG_NET_L2_ETHERNET)
    if (iface == NULL) {
        iface = get_ethernet_iface();
        if (iface == NULL) {
            LOG_ERR("No Ethernet network interface is available");
            ret = -ENODEV;
            goto out;
        }
    }
#endif

    if (iface == NULL) {
        LOG_ERR("No supported network interface is available");
        ret = -ENODEV;
        goto out;
    }

    ret = bring_iface_up(iface);
    if (ret < 0) {
        goto out;
    }

#if defined(CONFIG_WIFI) && defined(CONFIG_NET_L2_WIFI_MGMT)
    if (config->wifi != NULL) {
        ret = connect_wifi(iface, config->wifi);
        if (ret < 0) {
            goto out;
        }
    }
#endif

    ret = configure_ipv4(iface, config);

out:
    k_mutex_unlock(&net_start_mutex);
    return ret;
}
