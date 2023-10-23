#include "rx888_variant.h"
#include "librx888.h"
#include "rx888.h"

int rx888_init(void *dev)
{
    rx888_dev_t* dev_v = (rx888_dev_t *)dev;
    rx888_send_command(dev_v->dev_handle, STOPFX3, 0);
    rx888_send_command(dev_v->dev_handle, STARTADC, dev_v->sample_rate);
    rx888_send_command(dev_v->dev_handle, STARTFX3, 0);
    return 0;
}

int rx888_exit(void *dev)
{
    return 0;
}

int rx888_set_hf_attenuation(void *dev, double rf_gain)
{
    rx888_dev_t* dev_v = (rx888_dev_t *)dev;
    if (!dev_v)
        return -1;

    // Verify that rf_gain is one of the expected values
    if (rf_gain != 0.0 && rf_gain != -10.0 && rf_gain != -20.0)
        return -1;

    // Set the HF attenuation
    if (rf_gain == 0.0) {
        dev_v->gpio_state &= ~ATT_SEL0; // Clear the bit 13
        dev_v->gpio_state |= ATT_SEL1; // Set the bit 14
        } 
    else if (rf_gain == -10.0) {
        dev_v->gpio_state |= ATT_SEL0; // Set the bit 13
        dev_v->gpio_state |= ATT_SEL1; // Set the bit 14
        }
    else if (rf_gain == -20.0) {
        dev_v->gpio_state |= ATT_SEL0; // Set the bit 13
        dev_v->gpio_state &= ~ATT_SEL1; // Clear the bit 14
        }

    rx888_send_command(dev_v->dev_handle, GPIOFX3, dev_v->gpio_state);

    return 0;
}