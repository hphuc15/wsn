#include "utils_rtos.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void Utils_DelayMs(int32_t ms)
{
    if(ms < 0){
        vTaskDelay(portMAX_DELAY);
    } else {
        vTaskDelay((ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
    }
}