#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "init.h"

#define SLEEP_TIME_MS 1000

int main(void) {
    int ret = init();
    if (ret != 0) {
        printk("Init failed :(");
        return ret;
    }

    while (1) {
        gpio_pin_toggle_dt(&led);  // led is an extern
        printk("LED Toggled\n");
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
