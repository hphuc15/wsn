#ifndef BUTTON_H
#define BUTTON_H

void btn_set_short_press_cb(void (*cb)(void));
void btn_set_long_press_cb(void (*cb)(void));
int btn_init(void);

#endif /* BUTTON_H */