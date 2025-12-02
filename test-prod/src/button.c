#include "button.h"

struct gpio_callback button_cb;
volatile uint8_t counter = 0;
const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_ALIAS(button1), gpios);

void update_press(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{

    if(counter < 5)
    {
        counter++;
    }
    else
    {
        counter = 0;
    }
}