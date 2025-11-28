#ifndef BUTTON_H
#define BUTTON_H
#include <zephyr/drivers/gpio.h>

extern struct gpio_callback button_cb;
extern const struct gpio_dt_spec button1;
extern struct k_sem conn_sem;

void update_press(const struct device *dev,struct gpio_callback *cb,uint32_t pins);
#endif