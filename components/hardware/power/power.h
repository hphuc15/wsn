#ifndef POWER_H
#define POWER_H

#include <stdbool.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Power */

/**
 * @brief Initialize GPIOs and ADC, check battery before enabling sensor rail.
 *        If battery is too low, calls power_sleep() immediately.
 * @return 0 OK, -1 error
 */
int power_init(void);

/**
 * @brief Enter deep sleep, wake up via EXT0 (wake button).
 * @return -1 if config failed (normally doesn't return since it sleeps)
 */
int power_sleep(void);

/**
 * @brief Enable/disable status LED blinking.
 * @return 0 OK, -1 error
 */
int power_led_blink(bool on);

int power_led(bool on);

#endif /* POWER_H */