// Please keep these 2 lines at the beginning of each cpp module - tag and local log level
static const char* LOG_TAG = "Main";
#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include "defines.h"
#include "nvs_flash.h"
#include "sleep.h"

#include "mill_zigbee.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void run(void);
extern "C"
{
    void app_main(void)
    {
        run();
    }
}

using namespace mill_zigbee;

void configuartionFinished(bool success)
{
    if (success)
    {
        LOG_ERROR("Configuration finished success");
    }
    else
    {
        LOG_ERROR("Configuration finished failed");
    }
}

void factoryResetReceived()
{
    LOG_ERROR("Factory reset");
    auto* instance = MillZigbee::getInstance();
    instance->reset();
}

void newSystemMode(ESystemMode mode)
{
    LOG_ERROR("New system mode = %d", static_cast<uint8_t>(mode));
}

void newSetTemperature(float setTemperature)
{
    LOG_ERROR("New set temperature = %f", setTemperature);
}


TaskHandle_t task1 = nullptr;
TaskHandle_t task2 = nullptr;
TaskHandle_t task3 = nullptr;

void loggingTask1(void* pArgs)
{
    while (true)
    {
        int minRemaningBytes = uxTaskGetStackHighWaterMark2(task1);
        ESP_LOGE("Task1", "Minimal remaning number of bytes on stack task3 %d", minRemaningBytes);
        SLEEP_MS(1000);
    }
}


void loggingTask2(void* pArgs)
{
    while (true)
    {
        int minRemaningBytes = uxTaskGetStackHighWaterMark2(task2);
        ESP_LOGE("Task2", "Minimal remaning number of bytes on stack task3 %d", minRemaningBytes);
        SLEEP_MS(1000);
    }
}


void loggingTask3(void* pArgs)
{
    while (true)
    {
        int minRemaningBytes = uxTaskGetStackHighWaterMark2(task3);
        ESP_LOGE("Task3", "Minimal remaning number of bytes on stack task3 %d", minRemaningBytes);
        SLEEP_MS(1000);
    }
}


void run(void)
{
    xTaskCreate(loggingTask1, "Task1", 4096, nullptr, 5, &task1);
    xTaskCreate(loggingTask2, "Task2", 4096, nullptr, 5, &task2);
    xTaskCreate(loggingTask3, "Task3", 4096, nullptr, 5, &task3);

    SLEEP_MS(20000);
    ESP_ERROR_CHECK(nvs_flash_init());
    auto instance = MillZigbee::getInstance();
    instance->init(20.4, 13.5, ESystemMode::HEATING);
    instance->startZigbee();
    SLEEP_MS(3000);
    instance->setCallbcks(newSetTemperature, newSystemMode, factoryResetReceived, configuartionFinished);
    instance->startCommisioning();
    float temperature = 20;
    while (1)
    {
        LOG_DEBUG("Hello loop");
        SLEEP_MS(2000);

        // instance->setSystemMode(ESystemMode::HEATING);
        // SLEEP_MS(2000);
        // instance->setSystemMode(ESystemMode::OFF);

        // temperature++;
        // if (temperature > 30)
        //     temperature = -10;
        // instance->setLocalTemperature(temperature);
        // instance->setSetTemperature(temperature);
    }
}
