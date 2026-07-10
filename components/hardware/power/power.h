#ifndef POWER_H
#define POWER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Init status LED and wake button GPIOs, log wakeup cause.
 * @return 0 on success, -1 on error.
 */
int power_init(void);

/**
 * @brief Enter deep sleep, wake up via EXT0 (wake button).
 * @return -1 on config error (normally doesn't return since it sleeps).
 */
int power_sleep(void);

/** @brief Set status LED on/off. @return 0 on success, -1 on error. */
int power_led(bool on);

/** @brief Enable/disable status LED blinking task. @return 0 on success, -1 on error. */
int power_led_blink(bool on);

#endif /* POWER_H */