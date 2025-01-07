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


static const char* LOG_TAG = "Zigbee_mill";
#define LOG_AL_LEVEL ESP_LOG_INFO

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "defines.h"
#include "esp_err.h"
#include "esp_zigbee_attribute.h"
#include "esp_zigbee_core.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "mill_zigbee.h"
#include <cstdint>

/* Zigbee configuration */
bool     INSTALLCODE_POLICY_ENABLE = false; /* enable the install code policy for security */
uint8_t  ED_AGING_TIMEOUT          = ESP_ZB_ED_AGING_TIMEOUT_64MIN;
uint32_t ED_KEEP_ALIVE             = 3000; /* 3000 millisecond */
uint8_t  HA_THERMOSTAT_ENDPOINT    = 1;    /* esp thermostat device endpoint */

/* Basic manufacturer information */
constexpr const char* MANUFACTURER_NAME = "\x04"
                                          "Mill"; /* Customized manufacturer name */
constexpr const char* MODEL_IDENTIFIER = "\x10"
                                         "Panel gen4"; /* Customized model identifier */

constexpr const char*    TAG                           = "ESP_HA_ON_OFF_SWITCH";
constexpr const uint8_t  OPERATION_MODE_HEATING_ONLY   = 2;
constexpr const uint8_t  SYSTEM_MODE_HEATING           = 4;
constexpr const uint8_t  SYSTEM_MODE_OFF               = 0;
constexpr const uint16_t ZIGBEE_TEMPERATURE_MULTIPLIER = 100; // We need to multiply value by 100 when
constexpr const uint32_t ZIGBEE_USE_ALL_CHANNELS       = 0x07FFF800;
int                      DEVICE_CLASS                  = 0x02;
int                      DEVICE_TYPE                   = 0x10;
constexpr const float    HEATING_MAX                   = 35.0f;
constexpr const float    HEATING_MIN                   = 5.0f;
constexpr const uint16_t OCUPIED_COOLING_SETPOINT      = 3600;
constexpr const uint8_t  SET_SETPOINT_HEAT_ONLY        = 0;
constexpr const uint8_t  SET_SETPOINT_BOTH             = 2;
constexpr const uint8_t  POWER_SOURCE_CONFIGURATION    = 1;


namespace mill_zigbee
{
MillZigbee* pInstance = nullptr;
}

// This function is requeired by zigbee
void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct)
{
    mill_zigbee::pInstance->zigbeeSignalsLoop(signal_struct);
}


namespace mill_zigbee
{


MillZigbee* MillZigbee::getInstance()
{
    if (pInstance == nullptr)
    {
        pInstance = new MillZigbee();
    }

    return pInstance;
}


esp_err_t MillZigbee::zigbeeActionHandlerStatic(esp_zb_core_action_callback_id_t callbackId, const void* message)
{
    return pInstance->zigbeeActionHandler(callbackId, message);
}

esp_err_t MillZigbee::zigbeeActionHandler(esp_zb_core_action_callback_id_t callbackId, const void* message)
{
    esp_err_t result = ESP_OK;
    switch (callbackId)
    {
        case ESP_ZB_CORE_THERMOSTAT_VALUE_CB_ID:
        {
            LOG_ERROR("Theremo cb");
            result = thermostatClusterMessageHandler((esp_zb_zcl_thermostat_value_message_t*)message);
            break;
        }
        case ESP_ZB_CORE_THERMOSTAT_WEEKLY_SCHEDULE_SET_CB_ID:
        {
            result = thermostatWeeklyProgramSetCluster((esp_zb_zcl_thermostat_weekly_schedule_set_message_t*)message);
            LOG_ERROR("Theremo WEEKLY");
            break;
        }
        case ESP_ZB_CORE_CMD_THERMOSTAT_GET_WEEKLY_SCHEDULE_RESP_CB_ID:
        {
            LOG_ERROR("Theremo get WEEKLY");
            break;
        }
        case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        {
            result = setAttrbibuteCallback((esp_zb_zcl_set_attr_value_message_t*)message);
            LOG_ERROR("SET ATTRIBUTE");
            break;
        }
        case ESP_ZB_CORE_BASIC_RESET_TO_FACTORY_RESET_CB_ID:
        {
            handleFactoryReset();
            break;
        }
        default:
        {
            LOG_ERROR("Unkown action received %d", callbackId);
            break;
        }
    }
    LOG_ERROR("Return value = %d", result);
    return result;
}

void MillZigbee::zigbeeSignalsLoop(esp_zb_app_signal_t* pZigbeeSignal)
{
    uint32_t*                pAppSignal  = pZigbeeSignal->p_app_signal;
    esp_err_t                errorStatus = pZigbeeSignal->esp_err_status;
    esp_zb_app_signal_type_t signalType  = static_cast<esp_zb_app_signal_type_t>(*pAppSignal);
    switch (signalType)
    {
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            LOG_ERROR("ZDO skip startup");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
            break;
        case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        {
            LOG_ERROR("Reboot/First start, zigbee ready");
            m_started = true;
            if (isConfigured())
            {
                LOG_ERROR("Start existing network");
                esp_zb_bdb_open_network(180);
            }
            break;
        }
        case ESP_ZB_BDB_SIGNAL_FORMATION:
        {
            LOG_ERROR("Network formed we can start commisioning");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            break;
        }
        case ESP_ZB_BDB_SIGNAL_STEERING:
        {
            bool success = errorStatus == ESP_OK;
            if (success)
            {
                LOG_ERROR("Networking steering started");
            }
            else
            {
                LOG_ERROR("Networking stering FAILED");
            }
            m_commisioningCompletedCallback(success);

            break;
        }
        case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        {
            LOG_ERROR("Device connected");
            break;
        }
        case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS:
        {
            LOG_ERROR("NWK permit join status");
            if (errorStatus == ESP_OK)
            {
                if (*(uint8_t*)esp_zb_app_signal_get_params(pAppSignal))
                {
                    ESP_LOGI(
                        TAG,
                        "Network(0x%04hx) is open for %d seconds",
                        esp_zb_get_pan_id(),
                        *(uint8_t*)esp_zb_app_signal_get_params(pAppSignal));
                }
                else
                {
                    ESP_LOGW(TAG, "Network(0x%04hx) closed, devices joining not allowed.", esp_zb_get_pan_id());
                }
            }
            break;
        }
        case ESP_ZB_NLME_STATUS_INDICATION:
        {
            LOG_ERROR("Network lost, device should reconnect autmatically");
            break;
        }
        default:
            LOG_ERROR("Unkown signal %d", signalType);
            ESP_LOGI(
                TAG,
                "ZDO signal: %s (0x%x), status: %s",
                esp_zb_zdo_signal_to_string(signalType),
                signalType,
                esp_err_to_name(errorStatus));
            break;
    }
}

bool MillZigbee::isConfigured() const
{
    return !esp_zb_bdb_is_factory_new();
}

bool MillZigbee::startCommisioning()
{
    if (isConfigured())
    {
        LOG_ERROR("Already configured");
        return false;
    }

    if (!m_started)
    {
        LOG_ERROR("Zigbee not started");
        return false;
    }

    auto result = esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
    if (result != ESP_OK)
    {
        LOG_ERROR("Failed to start commisioninng");
        return false;
    }
    return true;
}

void MillZigbee::setCallbcks(
    FNewSetpointReceivedCallback   newSetpointCallback,
    FNewSystemModeReceivedCallback newSystemModeCallback,
    FFactoryResetRequestCallbck    factoryResetCallback,
    FCommisioningCompleted         commisioningCompletedCallback)
{
    m_newSetpointReceivedCallback          = newSetpointCallback;
    m_newSystemModeReceivedCallback        = newSystemModeCallback;
    m_factoryResetRequestedRevicedCallback = factoryResetCallback;
    m_commisioningCompletedCallback        = commisioningCompletedCallback;
}

void MillZigbee::perform(void* params)
{
    auto* millZigbeeInstance = static_cast<MillZigbee*>(params);
    millZigbeeInstance->performZigbee();
}

void MillZigbee::performZigbee()
{
    /* Initialize Zigbee stack */
    esp_zb_cfg_t zb_nwk_cfg               = {};
    zb_nwk_cfg.esp_zb_role                = ESP_ZB_DEVICE_TYPE_ED;
    zb_nwk_cfg.install_code_policy        = INSTALLCODE_POLICY_ENABLE;
    zb_nwk_cfg.nwk_cfg.zed_cfg.ed_timeout = ED_AGING_TIMEOUT;
    zb_nwk_cfg.nwk_cfg.zed_cfg.keep_alive = ED_KEEP_ALIVE;

    esp_zb_init(&zb_nwk_cfg);

    /* Create customized thermostat endpoint */
    esp_zb_thermostat_cfg_t thermostatConfig                      = ESP_ZB_DEFAULT_THERMOSTAT_CONFIG();
    thermostatConfig.thermostat_cfg.control_sequence_of_operation = OPERATION_MODE_HEATING_ONLY;
    thermostatConfig.thermostat_cfg.occupied_heating_setpoint     = m_setTemperature * ZIGBEE_TEMPERATURE_MULTIPLIER;
    thermostatConfig.thermostat_cfg.local_temperature             = m_localTemperature * ZIGBEE_TEMPERATURE_MULTIPLIER;
    thermostatConfig.thermostat_cfg.system_mode                   = static_cast<uint8_t>(m_systemMode);
    thermostatConfig.thermostat_cfg.occupied_cooling_setpoint     = OCUPIED_COOLING_SETPOINT;

    esp_zb_ep_list_t* pThermostatEndpoint = createThermostatEndpoint(HA_THERMOSTAT_ENDPOINT, &thermostatConfig);
    /* Register the device */
    auto result = esp_zb_device_register(pThermostatEndpoint);
    LOG_ERROR("Register %d", result);
    esp_zb_core_action_handler_register(zigbeeActionHandlerStatic);
    result = esp_zb_set_primary_network_channel_set(ZIGBEE_USE_ALL_CHANNELS);
    LOG_ERROR("Channel %d", result);
    esp_zb_start(false);
    esp_zb_secur_network_min_join_lqi_set(1);
    while (true)
    {
        esp_zb_stack_main_loop_iteration();
    }
}


void MillZigbee::startZigbee()
{
    auto result = xTaskCreate(MillZigbee::perform, "Zigbee_main", 4096, this, 5, &m_taskHandle);
    if (result != pdPASS)
        LOG_ERROR("Failed to create a task: %s", "Zigbee_main");
}

void MillZigbee::setLocalTemperature(float localTemperature)
{
    int16_t temperatureZigbee = localTemperature * ZIGBEE_TEMPERATURE_MULTIPLIER;
    auto    status            = esp_zb_zcl_set_attribute_val(
        HA_THERMOSTAT_ENDPOINT,
        0X201,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        ESP_ZB_ZCL_ATTR_THERMOSTAT_LOCAL_TEMPERATURE_ID,
        &temperatureZigbee,
        false);
    LOG_ERROR("sTATUS = %d", status);
}

void MillZigbee::setSetTemperature(float setTemperature)
{
    int16_t temperatureZigbee = setTemperature * ZIGBEE_TEMPERATURE_MULTIPLIER;
    auto    status            = esp_zb_zcl_set_attribute_val(
        HA_THERMOSTAT_ENDPOINT,
        0X201,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        ESP_ZB_ZCL_ATTR_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_ID,
        &temperatureZigbee,
        false);
    LOG_ERROR("sTATUS  set temperature = %d %d", status, temperatureZigbee);
}

void MillZigbee::setSystemMode(ESystemMode systemMode)
{
    auto status = esp_zb_zcl_set_attribute_val(
        HA_THERMOSTAT_ENDPOINT,
        0X201,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        ESP_ZB_ZCL_ATTR_THERMOSTAT_SYSTEM_MODE_ID,
        &systemMode,
        false);
    LOG_ERROR("sTATUS  set system mode %d", status);
}

void MillZigbee::reset()
{
    LOG_WARNING("About to factory reset the device");
    esp_zb_factory_reset();
}

void MillZigbee::init(float localTemperature, float setTemperature, ESystemMode systemMode)
{
    esp_zb_radio_config_t radioConfig       = {};
    radioConfig.radio_mode                  = ZB_RADIO_MODE_NATIVE;
    esp_zb_host_config_t hostConfig         = {};
    hostConfig.host_connection_mode         = ZB_HOST_CONNECTION_MODE_NONE;
    esp_zb_platform_config_t platformConfig = {
        .radio_config = radioConfig,
        .host_config  = hostConfig,
    };
    auto result = esp_zb_platform_config(&platformConfig);
    if (result != ESP_OK)
    {
        LOG_ERROR("Failed to init zigbee platform");
    }
    m_localTemperature = localTemperature;
    m_setTemperature   = setTemperature;
    m_systemMode       = systemMode;
}

esp_err_t MillZigbee::setAttrbibuteCallback(const esp_zb_zcl_set_attr_value_message_t* pMessage)
{
    LOG_ERROR("Attribute received callback %u", pMessage->attribute.id);


    switch (pMessage->attribute.id)
    {
        case ESP_ZB_ZCL_ATTR_THERMOSTAT_SYSTEM_MODE_ID:
        {
            uint8_t systemMode = *static_cast<uint8_t*>(pMessage->attribute.data.value);
            LOG_ERROR("System mode");
            handleNewSystemMode(systemMode);
            break;
        }
        case ESP_ZB_ZCL_ATTR_THERMOSTAT_CONTROL_SEQUENCE_OF_OPERATION_ID:
        {
            LOG_ERROR("Handle sequence of operation");
            setSuquenceOfOperation();
            break;
        }
    }
    return ESP_OK;
}

esp_err_t MillZigbee::thermostatClusterMessageHandler(const esp_zb_zcl_thermostat_value_message_t* message)
{
    LOG_ERROR("Message received %d %d", message->heat_setpoint, message->mode);
    if (message->mode == SET_SETPOINT_HEAT_ONLY || message->mode == SET_SETPOINT_BOTH)
    {
        LOG_ERROR("Received valid temperature for independent device");
        m_setTemperature     = message->heat_setpoint;
        float setTemperature = m_setTemperature / ZIGBEE_TEMPERATURE_MULTIPLIER;
        m_newSetpointReceivedCallback(setTemperature);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t
MillZigbee::thermostatWeeklyProgramSetCluster(const esp_zb_zcl_thermostat_weekly_schedule_set_message_t* message)
{
    LOG_ERROR(
        "Message received %d %d %d %lu",
        message->day_of_week,
        message->mode_for_req,
        message->trans.transition_time,
        message->trans_status);

    return ESP_OK;
}

void MillZigbee::handleFactoryReset()
{
    LOG_INFO("Received factory reset");
    m_factoryResetRequestedRevicedCallback();
}

esp_zb_ep_list_t* MillZigbee::createThermostatEndpoint(uint8_t endpointId, esp_zb_thermostat_cfg_t* thermostatConfig)
{
    esp_zb_ep_list_t* endpointList = esp_zb_ep_list_create();

    esp_zb_endpoint_config_t endpointConfiguration = {
        .endpoint           = endpointId,
        .app_profile_id     = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id      = ESP_ZB_HA_THERMOSTAT_DEVICE_ID,
        .app_device_version = 1};
    thermostatConfig->basic_cfg.power_source = POWER_SOURCE_CONFIGURATION;
    auto* pClusterList                       = createThermostatCluster(thermostatConfig);
    esp_zb_ep_list_add_ep(endpointList, pClusterList, endpointConfiguration);
    return endpointList;
}

esp_zb_cluster_list_t* MillZigbee::createThermostatCluster(esp_zb_thermostat_cfg_t* thermostat)
{
    esp_zb_cluster_list_t*   pClusterList  = esp_zb_zcl_cluster_list_create();
    esp_zb_attribute_list_t* pBasicCluster = esp_zb_basic_cluster_create(&(thermostat->basic_cfg));
    esp_zb_basic_cluster_add_attr(
        pBasicCluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, const_cast<char*>(MANUFACTURER_NAME));
    esp_zb_basic_cluster_add_attr(
        pBasicCluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, const_cast<char*>(MODEL_IDENTIFIER));

    esp_zb_cluster_list_add_basic_cluster(pClusterList, pBasicCluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(
        pClusterList, esp_zb_identify_cluster_create(&(thermostat->identify_cfg)), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(
        pClusterList, esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);

    auto* pThermostatCluster = esp_zb_thermostat_cluster_create(&(thermostat->thermostat_cfg));
    esp_zb_cluster_list_add_thermostat_cluster(pClusterList, pThermostatCluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    int16_t heatingMinTemperature = HEATING_MIN * ZIGBEE_TEMPERATURE_MULTIPLIER;
    int16_t heatingMaxTemperature = HEATING_MAX * ZIGBEE_TEMPERATURE_MULTIPLIER;

    esp_zb_cluster_add_attr(
        pThermostatCluster,
        0x201,
        ESP_ZB_ZCL_ATTR_THERMOSTAT_ABS_MAX_HEAT_SETPOINT_LIMIT_ID,
        ESP_ZB_ZCL_ATTR_TYPE_S16,
        ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
        &heatingMaxTemperature);

    esp_zb_cluster_add_attr(
        pThermostatCluster,
        0x201,
        ESP_ZB_ZCL_ATTR_THERMOSTAT_MAX_HEAT_SETPOINT_LIMIT_ID,
        ESP_ZB_ZCL_ATTR_TYPE_S16,
        ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
        &heatingMaxTemperature);

    esp_zb_cluster_add_attr(
        pThermostatCluster,
        0x201,
        ESP_ZB_ZCL_ATTR_THERMOSTAT_ABS_MIN_HEAT_SETPOINT_LIMIT_ID,
        ESP_ZB_ZCL_ATTR_TYPE_S16,
        ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
        &heatingMinTemperature);

    esp_zb_cluster_add_attr(
        pThermostatCluster,
        0x201,
        ESP_ZB_ZCL_ATTR_THERMOSTAT_MIN_HEAT_SETPOINT_LIMIT_ID,
        ESP_ZB_ZCL_ATTR_TYPE_S16,
        ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
        &heatingMinTemperature);
    return pClusterList;
}

/**
 * @brief MillZigbee::setSuquenceOfOperation set private
 */
void MillZigbee::setSuquenceOfOperation()
{
    auto sequenceOfOperation = esp_zb_zcl_thermostat_control_sequence_of_operation_t::
        ESP_ZB_ZCL_THERMOSTAT_CONTROL_SEQ_OF_OPERATION_HEATING_ONLY;
    auto status = esp_zb_zcl_set_attribute_val(
        HA_THERMOSTAT_ENDPOINT,
        0X201,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        ESP_ZB_ZCL_ATTR_THERMOSTAT_CONTROL_SEQUENCE_OF_OPERATION_ID,
        &sequenceOfOperation,
        false);
    LOG_ERROR("sTATUS  set sequence of operation %d", status);
}

void MillZigbee::handleNewSystemMode(uint8_t systemMode)
{
    if (systemMode != SYSTEM_MODE_HEATING && systemMode != SYSTEM_MODE_OFF)
    {
        setSystemMode(m_systemMode);
        return;
    }

    m_systemMode = static_cast<ESystemMode>(systemMode);
    m_newSystemModeReceivedCallback(m_systemMode);
    return;
}

} // namespace mill_zigbee
