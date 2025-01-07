#ifndef DEFINES_H
#define DEFINES_H


#include "esp_log.h"

#include <stdint.h>
#include <stdbool.h>


#define ENABLE_PRINTS_AND_LOGS  // TODO - move to CMake as an option
#ifdef ENABLE_PRINTS_AND_LOGS
    #define LOG_DEBUG(...)       ESP_LOGD(LOG_TAG, __VA_ARGS__)
    #define LOG_INFO(...)        ESP_LOGI(LOG_TAG, __VA_ARGS__)
    #define LOG_WARNING(...)     ESP_LOGW(LOG_TAG, __VA_ARGS__)
    #define LOG_ERROR(...)       ESP_LOGE(LOG_TAG, __VA_ARGS__)
#else
    #define LOG_DEBUG(...)       do {} while(0)
    #define LOG_INFO(...)        do {} while(0)
    #define LOG_WARNING(...)     ESP_LOGW(LOG_TAG, __VA_ARGS__)
    #define LOG_ERROR(...)       ESP_LOGE(LOG_TAG, __VA_ARGS__)
#endif


#define IS_DEBUG_BUILD  // TODO - move to CMake as an option
#ifdef IS_DEBUG_BUILD
    #define DEBUG_ASSERT(x) do { if (!(x)) { \
        LOG_ERROR("Assertion failed. Function %s, file %s, line %d.\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
        while (1) { } } } while(0)
#else
    #define DEBUG_ASSERT(x) do {} while(0)
#endif


#define MILLISEC_IN_SEC 1000
#define USEC_IN_MILLISEC 1000

#define DEFAULT_STACK_SIZE 2048


#endif // DEFINES_H
