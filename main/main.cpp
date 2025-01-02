// Please keep these 2 lines at the beginning of each cpp module - tag and local log level
static const char* LOG_TAG = "Main";
#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include "defines.h"
#include "nvs_flash.h"
#include "sleep.h"

#include "zigbee_example.h"

void run(void);
extern "C"
{
    void app_main(void)
    {
        run();
    }
}

void configuartionFinished()
{
    LOG_ERROR("Configuration finished");
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


void run(void)
{
    LOG_INFO("Hello from main!");


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
