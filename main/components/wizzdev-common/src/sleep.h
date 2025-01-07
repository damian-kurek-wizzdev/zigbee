#ifndef SLEEP_H
#define SLEEP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// needs to be multiple of 10 ms, otherwise it will be rounded down!
#define SLEEP_MS(X_MS) vTaskDelay(((X_MS >= 10) ? X_MS : 10) / portTICK_PERIOD_MS)


#endif  // SLEEP_H
