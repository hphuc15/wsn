#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdbool.h>

#include "io.h"

/** @brief Init power (incl. wake button GPIO) and button driver. @return 0 on success, -1 on error. */
int  hardware_init(void);

/** @brief Enter deep sleep. @return -1 on error (normally doesn't return). */
int  hardware_shutdown(void);

/** @brief Set callback for button short press. */
void hardware_set_btn_short_cb(void (*cb)(void));

/** @brief Set callback for button long press. */
void hardware_set_btn_long_cb(void (*cb)(void));

/** @brief Enable/disable status LED blinking. @return 0 on success, -1 on error. */
int  hardware_led_blink(bool on);

/** @brief Set status LED on/off. @return 0 on success, -1 on error. */
int  hardware_led(bool on);

#endif /* HARDWARE_H */