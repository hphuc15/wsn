#include "hardware.h"
#include "io.h"
#include "power.h"

int hardware_init(void)
{
    int ret = 0;
    ret |= power_init(); /* This function has already include button gpio initialization, may be i will move this feature to btn_init() */
    ret |= btn_init();

    return (ret != 0) ? -1 : 0;
}

int hardware_shutdown(void)
{
    return power_sleep();
}

void hardware_set_btn_short_cb(void (*cb)(void))
{
    btn_set_short_press_cb(cb);
}

void hardware_set_btn_long_cb(void (*cb)(void))
{
    btn_set_long_press_cb(cb);
}

int hardware_led_blink(bool on)
{
    return power_led_blink(on);
}

int hardware_led(bool on){
    return power_led(on);
}