#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS   500

/* Pull the led0 specification from the devicetree overlay */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
    /* Verify that the hardware driver is initialized and ready */
    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    /* Configure the pin as an output active pin */
    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

    while (1) {
        /* Safely toggle the state of the pin */
        gpio_pin_toggle_dt(&led);
        k_msleep(SLEEP_TIME_MS);
    }
    
    return 0;
}
