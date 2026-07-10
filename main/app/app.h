#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Init hardware, network, and receiver; start forward_task. @return 0 on success, -1 on error. */
int  app_init(void);

/** @brief Main loop hook. Currently unused; forwarding is event-driven. */
void app_run(void);

#endif /* APP_H */