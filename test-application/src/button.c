#include "button.h"
#include "bluetooth.h"

struct gpio_callback button_cb;
uint8_t counter = 0;
uint8_t button_presses = 0;
const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_ALIAS(button1), gpios);

void update_press(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    button_presses++;
    my_ble_send_button(counter);

    if(button_presses > 1 && counter < 5)
    {
        counter++;
        my_ble_send_button(counter);
    }
    else if(button_presses == 255)
    {
        button_presses = 0;
    }
    else
    {
        counter = 0;
        button_presses++;
        my_ble_send_button(counter);
    }
}