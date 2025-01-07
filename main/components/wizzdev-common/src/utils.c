// Please keep these 2 lines at the beginning of each cpp module - tag and local log level
static const char* LOG_TAG = "Utils";
#define LOG_LOCAL_LEVEL ESP_LOG_INFO


#include "utils.h"

#include "defines.h"


uint32_t multiplyBy2(uint32_t value)
{
    LOG_INFO("About to multiply by 2...");
    return value * 2;
}
