#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdbool.h>

#include "power.h"
#include "io.h"

int  hardware_init(void);
int  hardware_shutdown(void);

void hardware_set_btn_short_cb(void (*cb)(void));
void hardware_set_btn_long_cb(void (*cb)(void));
int  hardware_led_blink(bool on);

#endif /* HARDWARE_H */