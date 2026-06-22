#include "app.h"
#include "utilities.h"

void app_main(void)
{
    if (app_init() != 0) {
        Utils_LogE("[MAIN]", "app_init failed, halting.");
        while (1){
            Utils_DelayMs(-1);
        }
    }

    app_run();
}