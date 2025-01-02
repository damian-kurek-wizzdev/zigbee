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
#include <functional>

enum class ESystemMode : int8_t
{
    HEATING = 4,
    OFF     = 0
};


typedef std::function<void(float setTemperature)>   FNewSetpointReceivedCallback;
typedef std::function<void(ESystemMode systemMode)> FNewSystemModeReceivedCallback;
typedef std::function<void()>                       FFactoryResetRequestCallbck;
typedef std::function<void()>                       FCommisioningCompleted;


class MillZigbee
{
public:
    MillZigbee() = default;

    static MillZigbee* getInstance();

    static esp_err_t zigbeeActionHandlerStatic(esp_zb_core_action_callback_id_t callbackId, const void* message);
    esp_err_t        zigbeeActionHandler(esp_zb_core_action_callback_id_t callbackId, const void* message);

    void zigbeeSignalsLoop(esp_zb_app_signal_t* signal);

    bool isConfigured() const;
    bool startCommisioning();

    void setCallbcks(
        FNewSetpointReceivedCallback   newSetpointCallback,
        FNewSystemModeReceivedCallback newSystemModeCallback,
        FFactoryResetRequestCallbck    factoryResetCallback,
        FCommisioningCompleted         commisioningCompletedCallback);

#if !TESTING
    static void perform(void* params);
    void        performZigbee();
    void        init(float localTemperature, float setTemperature, ESystemMode systemMode);
    void        startZigbee();
#endif // !TESTING

    void setLocalTemperature(float localTemperature);
    void setSetTemperature(float setTemperature);
    void setSystemMode(ESystemMode systemMode);
    void reset();

private:
#if !TESTING
    TaskHandle_t m_taskHandle = nullptr;
#endif // !TESTING

    int16_t     m_localTemperature = 0;
    uint16_t    m_setTemperature   = 0;
    ESystemMode m_systemMode       = ESystemMode::OFF;
    bool        m_started          = false;

    FNewSetpointReceivedCallback   m_newSetpointReceivedCallback          = {};
    FNewSystemModeReceivedCallback m_newSystemModeReceivedCallback        = {};
    FFactoryResetRequestCallbck    m_factoryResetRequestedRevicedCallback = {};
    FCommisioningCompleted         m_commisioningCompletedCallback        = {};

    esp_err_t attributeReportingHandler(const esp_zb_zcl_report_attr_message_t* message);

    esp_err_t setAttrbibuteCallback(const esp_zb_zcl_set_attr_value_message_t* pMessage);
    esp_err_t appAttributeHandler(uint16_t cluster_id, const esp_zb_zcl_attribute_t* attribute);

    esp_err_t thermostatClusterMessageHandler(const esp_zb_zcl_thermostat_value_message_t* message);
    esp_err_t thermostatWeeklyProgramSetCluster(const esp_zb_zcl_thermostat_weekly_schedule_set_message_t* message);

    void handleFactoryReset();

    esp_zb_ep_list_t*      createThermostatEndpoint(uint8_t endpointId, esp_zb_thermostat_cfg_t* thermostatConfig);
    esp_zb_cluster_list_t* createThermostatCluster(esp_zb_thermostat_cfg_t* thermostat);

    void setSuquenceOfOperation();
    void handleNewSystemMode(uint8_t systemMode);
};

#endif // ZIGBEE_EXAMPLE_H
