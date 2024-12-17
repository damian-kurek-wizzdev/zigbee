// Please keep these 2 lines at the beginning of each cpp module - tag and local log level
static const char* LOG_TAG = "Main";
#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include "defines.h"
#include "nvs_flash.h"
#include "sleep.h"

#include "esp_zigbee_core.h"
#include "zigbee_example.h"

void run(void);
#define ESP_ZB_DEFAULT_RADIO_CONFIG()       \
    {                                       \
        .radio_mode = ZB_RADIO_MODE_NATIVE, \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                          \
    {                                                         \
        .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE, \
    }

extern "C" {
void app_main(void)
{
    run();
}
}

void run(void)
{
    LOG_INFO("Hello from main!");

    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
    xTaskCreate(commisionTask, "Zigbee_main", 4096, NULL, 5, NULL);

    while (1) {
        LOG_DEBUG("Hello loop");
        SLEEP_MS(1000);
    }
}
