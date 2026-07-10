#ifndef BUTTON_H
#define BUTTON_H

/** @brief Set callback for button short press. */
void btn_set_short_press_cb(void (*cb)(void));

/** @brief Set callback for button long press. */
void btn_set_long_press_cb(void (*cb)(void));

/** @brief Init button GPIO polling task. @return 0 on success. */
int btn_init(void);

#endif /* BUTTON_H */