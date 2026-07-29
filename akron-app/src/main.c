#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define SLEEP_TIME_MS 1000

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

    while (1) {
        gpio_pin_toggle_dt(&led);
        printk("LED Toggled\n");
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
