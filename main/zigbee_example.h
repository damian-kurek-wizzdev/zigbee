/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Zigbee customized client Example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

#ifndef ZIGBEE_EXAMPLE_H
#define ZIGBEE_EXAMPLE_H

#include <cstdint>
#if !TESTING
// Forward declaration
struct tskTaskControlBlock;
// Type alias for a pointer to tskTaskControlBlock
typedef struct tskTaskControlBlock* TaskHandle_t;
#endif // !TESTING

#include "esp_zigbee_core.h"

enum class ESystemMode : int8_t
{
    HEATING = 4,
    OFF     = 0
};


class MillZigbee
{
public:
    MillZigbee();

    static MillZigbee* getInstance();

    static esp_err_t zigbeeActionHandlerStatic(esp_zb_core_action_callback_id_t callbackId, const void* message);
    esp_err_t        zigbeeActionHandler(esp_zb_core_action_callback_id_t callbackId, const void* message);


#if !TESTING
    static void perform(void* params);
    void        performZigbee();
    void        init(float localTemperature, float setTemperature, ESystemMode systemMode);
    void        startZigbee();
#endif // !TESTING


private:
#if !TESTING
    TaskHandle_t m_taskHandle = nullptr;
#endif // !TESTING

    int16_t     m_localTemperature;
    int16_t     m_setTemperature;
    ESystemMode m_systemMode;


    esp_err_t setAttrbibuteCallback(const esp_zb_zcl_set_attr_value_message_t* pMessage);
};

void esp_zb_task(void* pvParameters);
void commisionTask(void* pvParameters);

#endif // ZIGBEE_EXAMPLE_H
