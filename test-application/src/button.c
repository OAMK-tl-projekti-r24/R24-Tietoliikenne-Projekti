// button.c
#include "button.h"
#include "bluetooth.h"

// Definitions — no 'static', so linker can see them
struct gpio_callback button_cb;
uint8_t counter = 0;
const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_ALIAS(button1), gpios);

void update_press(const struct device *dev,
                  struct gpio_callback *cb,
                  uint32_t pins)
{
    counter++;
    my_ble_send_button(counter);
}