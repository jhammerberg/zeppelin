#include <helium/net_start.h>
#include <helium/time_sync.h>
#include <helium/udp_server.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define SLEEP_TIME_MS 1000

#if defined(CONFIG_WIFI) && defined(CONFIG_NET_L2_WIFI_MGMT)
#define WIFI_SSID "something"
#define WIFI_PSK "something"
#define WIFI_TIMEOUT 60000  // milliseconds

static const struct wifi_config wifi = {
    .ssid = WIFI_SSID,
    .password = WIFI_PSK,
    .timeout = WIFI_TIMEOUT,
};

static const struct network_config network = {
    .prefer_dhcp = 1,
    .wifi = &wifi,
    .static_ip = NULL,
};
#else
static const struct static_ip_config static_ip = {
    .ip = CONFIG_NET_CONFIG_MY_IPV4_ADDR,
    .netmask = CONFIG_NET_CONFIG_MY_IPV4_NETMASK,
    .gateway = CONFIG_NET_CONFIG_MY_IPV4_GW,
};

static const struct network_config network = {
    .prefer_dhcp = 0,
    .wifi = NULL,
    .static_ip = &static_ip,
};
#endif

// Pull the led0 specification from the devicetree overlay
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(button0), gpios);

// Button callback data and handler
static struct gpio_callback button_cb_data;
void button_pressed_handler(const struct device* port, struct gpio_callback* cb,
                            gpio_port_pins_t pins) {
    printk("Button Pressed!\n");
}

int main(void) {
    // Verify that the hardware driver is initialized and ready
    if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&button)) return -1;

    // Configure the pin as an output active pin
    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) return -1;
    if (gpio_pin_configure_dt(&button, GPIO_INPUT) < 0) return -1;

    // Configure the button interrupt
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&button_cb_data, button_pressed_handler, BIT(button.pin));
    gpio_add_callback_dt(&button, &button_cb_data);

    if (net_start(&network) < 0) {
        gpio_pin_set_dt(&led, 1);  // set LED on to show error
        return -1;
    }

    // Synchronize the system clock from a network time server, now that the
    // network connection has been acquired.
    time_sync_now();

    // Start the echo UDP server on its own thread
    udp_echo_server_start();

    while (1) {
        gpio_pin_toggle_dt(&led);
        printk("LED Toggled\n");
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
