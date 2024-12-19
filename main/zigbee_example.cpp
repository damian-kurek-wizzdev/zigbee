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

#include "esp_zigbee_core.h"
#include "zcl/esp_zigbee_zcl_basic.h"
#include "zdo/esp_zigbee_zdo_common.h"
static const char* LOG_TAG = "Zigbee_mill";
#define LOG_AL_LEVEL ESP_LOG_INFO

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "defines.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"
#include "sleep.h"
#include "string.h"
#include "zcl/esp_zigbee_zcl_command.h"
#include "zcl/esp_zigbee_zcl_commissioning.h"
#include "zcl/esp_zigbee_zcl_common.h"
#include "zcl/esp_zigbee_zcl_thermostat.h"
#include "zigbee_example.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Zigbee configuration */
#define INSTALLCODE_POLICY_ENABLE     false /* enable the install code policy for security */
#define ED_AGING_TIMEOUT              ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE                 3000       /* 3000 millisecond */
#define HA_ONOFF_SWITCH_ENDPOINT      1          /* esp switch device endpoint */
#define ESP_ZB_PRIMARY_CHANNEL_MASK   (1l << 11) /* Zigbee primary channel mask use in the example */
#define ESP_ZB_SECONDARY_CHANNEL_MASK (1l << 13) /* Zigbee primary channel mask use in the example */
#define HA_THERMOSTAT_ENDPOINT        1          /* esp thermostat device endpoint */

/* Basic manufacturer information */
constexpr const char* ESP_MANUFACTURER_NAME = "\x04"
                                              "Mill"; /* Customized manufacturer name */
constexpr const char* ESP_MODEL_IDENTIFIER = "\x10"
                                             "Panel gen4"; /* Customized model identifier */

constexpr const char*    TAG                           = "ESP_HA_ON_OFF_SWITCH";
constexpr const uint8_t  OPERATION_MODE_HEATING_ONLY   = 2;
constexpr const uint8_t  SYSTEM_MODE_HEATING           = 4;
constexpr const uint8_t  SYSTEM_MODE_AUTO              = 1;
constexpr const uint8_t  SYSTEM_MODE_OFF               = 0;
constexpr const uint16_t ZIGBEE_TEMPERATURE_MULTIPLIER = 100; // We need to multiply value by 100 when
constexpr const uint32_t ZIGBEE_USE_ALL_CHANNELS       = 0x07FFF800;
int                      DEVICE_CLASS                  = 0x02;
int                      DEVICE_TYPE                   = 0x10;

typedef struct zbstring_s
{
    uint8_t len;
    char    data[];
} ESP_ZB_PACKED_STRUCT zbstring_t;

static float zb_s16_to_temperature(int16_t value)
{
    return 1.0 * value / 100;
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    LOG_ERROR("BDB top level commisioning cb");
    ESP_RETURN_ON_FALSE(
        esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK,
        ,
        TAG,
        "Failed to start Zigbee bdb commissioning");
}

MillZigbee* pInstance = nullptr;

MillZigbee* MillZigbee::getInstance()
{
    if (pInstance == nullptr)
    {
        pInstance = new MillZigbee();
    }
    return pInstance;
}


esp_err_t initEspZigbeeExternC(esp_zb_platform_config_t* config)
{
    return esp_zb_platform_config(config);
}


void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct)
{
    pInstance->zigbeeSignalsLoop(signal_struct);
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
        case ESP_ZB_CORE_REPORT_ATTR_CB_ID:
            LOG_ERROR("Attrubyte report handler");
            result = attributeReportingHandler((esp_zb_zcl_report_attr_message_t*)message);
            break;
        case ESP_ZB_CORE_CMD_READ_ATTR_RESP_CB_ID:
            LOG_ERROR("Read attr response");
            result = readAttributeResponeHandler((esp_zb_zcl_cmd_read_attr_resp_message_t*)message);
            break;
        case ESP_ZB_CORE_CMD_REPORT_CONFIG_RESP_CB_ID:
            LOG_ERROR("Configure report respons handler");
            result = configureReportResponseHandler((esp_zb_zcl_cmd_config_report_resp_message_t*)message);
            break;
        case ESP_ZB_CORE_THERMOSTAT_VALUE_CB_ID:
            LOG_ERROR("Theremo cb");
            result = thermostatClusterMessageHandler((esp_zb_zcl_thermostat_value_message_t*)message);
            break;
        case ESP_ZB_CORE_THERMOSTAT_WEEKLY_SCHEDULE_SET_CB_ID:
            result = thermostatWeeklyProgramSetCluster((esp_zb_zcl_thermostat_weekly_schedule_set_message_t*)message);
            LOG_ERROR("Theremo WEEKLY");
            break;

        case ESP_ZB_CORE_CMD_THERMOSTAT_GET_WEEKLY_SCHEDULE_RESP_CB_ID:
            LOG_ERROR("Theremo get WEEKLY");
            break;

        case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
            result = setAttrbibuteCallback((esp_zb_zcl_set_attr_value_message_t*)message);
            LOG_ERROR("SET ATTRIBUTE");

        default:
            LOG_ERROR("Unkown callback received %d", callbackId);
            break;
    }
    LOG_ERROR("Return value = %d", result);
    return result;
}

void MillZigbee::zigbeeSignalsLoop(esp_zb_app_signal_t* pZigbeeSignal)
{
    uint32_t*                                p_sg_p           = pZigbeeSignal->p_app_signal;
    esp_err_t                                err_status       = pZigbeeSignal->esp_err_status;
    esp_zb_app_signal_type_t                 sig_type         = static_cast<esp_zb_app_signal_type_t>(*p_sg_p);
    esp_zb_zdo_signal_device_annce_params_t* dev_annce_params = NULL;
    switch (sig_type)
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

            // if (err_status == ESP_OK)
            // {
            //     LOG_ERROR("Network formed we can start commisioning");

            //     esp_zb_ieee_addr_t extended_pan_id;
            //     esp_zb_get_extended_pan_id(extended_pan_id);

            //     ESP_LOGI(
            //         TAG,
            //         "Formed network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: "
            //         "0x%04hx, Channel:%d, Short Address: 0x%04hx)",
            //         extended_pan_id[7],
            //         extended_pan_id[6],
            //         extended_pan_id[5],
            //         extended_pan_id[4],
            //         extended_pan_id[3],
            //         extended_pan_id[2],
            //         extended_pan_id[1],
            //         extended_pan_id[0],
            //         esp_zb_get_pan_id(),
            //         esp_zb_get_current_channel(),
            //         esp_zb_get_short_address());
            //     esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);

            //     // bdb_start_top_level_commissioning_cb()
            // }
            // else
            // {
            //     LOG_ERROR("sTART TOP LEVEL COMMISIONING");
            //     ESP_LOGI(TAG, "Restart network formation (status: %s)", esp_err_to_name(err_status));
            //     esp_zb_scheduler_alarm(
            //         (esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_FORMATION,
            //         1000);
            // }
            // break;
        }
        case ESP_ZB_BDB_SIGNAL_STEERING:
            if (err_status == ESP_OK)
            {
                ESP_LOGI(TAG, "Network steering started");
            }
            break;
        case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE:
            LOG_ERROR("Device connected");
            LOG_ERROR("Device signal device annce");
            dev_annce_params = (esp_zb_zdo_signal_device_annce_params_t*)esp_zb_app_signal_get_params(p_sg_p);
            ESP_LOGI(TAG, "New device commissioned or rejoined (short: 0x%04hx)", dev_annce_params->device_short_addr);
            esp_zb_zdo_match_desc_req_param_t cmd_req;
            cmd_req.dst_nwk_addr     = dev_annce_params->device_short_addr;
            cmd_req.addr_of_interest = dev_annce_params->device_short_addr;
            break;
        case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS:
            LOG_ERROR("NWK permit join status");
            if (err_status == ESP_OK)
            {
                if (*(uint8_t*)esp_zb_app_signal_get_params(p_sg_p))
                {
                    ESP_LOGI(
                        TAG,
                        "Network(0x%04hx) is open for %d seconds",
                        esp_zb_get_pan_id(),
                        *(uint8_t*)esp_zb_app_signal_get_params(p_sg_p));
                }
                else
                {
                    ESP_LOGW(TAG, "Network(0x%04hx) closed, devices joining not allowed.", esp_zb_get_pan_id());
                }
            }
            break;
        case ESP_ZB_NLME_STATUS_INDICATION:
        {
            LOG_ERROR("Network lost, device should reconnect autmatically");
        }
        default:
            ESP_LOGI(
                TAG,
                "ZDO signal: %s (0x%x), status: %s",
                esp_zb_zdo_signal_to_string(sig_type),
                sig_type,
                esp_err_to_name(err_status));
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

    esp_zb_ep_list_t* pThermostatEndpoint = createThermostatEndpoint(HA_THERMOSTAT_ENDPOINT, &thermostatConfig);
    /* Register the device */
    auto result = esp_zb_device_register(pThermostatEndpoint);
    LOG_ERROR("Register %d", result);

    esp_zb_core_action_handler_register(zigbeeActionHandlerStatic);
    result = esp_zb_set_primary_network_channel_set(ZIGBEE_USE_ALL_CHANNELS);
    LOG_ERROR("Channel %d", result);
    ESP_ERROR_CHECK(esp_zb_start(false));
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

esp_err_t MillZigbee::attributeReportingHandler(const esp_zb_zcl_report_attr_message_t* message)
{
    LOG_ERROR("Reporting Handler");
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(
        message->status == ESP_ZB_ZCL_STATUS_SUCCESS,
        ESP_ERR_INVALID_ARG,
        TAG,
        "Received message: error status(%d)",
        message->status);
    ESP_LOGI(
        TAG,
        "Received report from address(0x%x) src endpoint(%d) to dst endpoint(%d) cluster(0x%x)",
        message->src_address.u.short_addr,
        message->src_endpoint,
        message->dst_endpoint,
        message->cluster);
    appAttributeHandler(message->cluster, &message->attribute);
    return ESP_OK;
}

esp_err_t MillZigbee::setAttrbibuteCallback(const esp_zb_zcl_set_attr_value_message_t* pMessage)
{
    LOG_ERROR("Attribute received callback %u", pMessage->attribute.id);
    if (pMessage->attribute.id == ESP_ZB_ZCL_ATTR_THERMOSTAT_SYSTEM_MODE_ID)
    {
        LOG_ERROR("System mode");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t MillZigbee::appAttributeHandler(uint16_t cluster_id, const esp_zb_zcl_attribute_t* attribute)
{
    LOG_ERROR("ATTR handler");
    /* Basic cluster attributes */
    if (cluster_id == ESP_ZB_ZCL_CLUSTER_ID_BASIC)
    {
        if (attribute->id == ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID &&
            attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_CHAR_STRING && attribute->data.value)
        {
            LOG_ERROR("Received manufacturer name");
            zbstring_t* zbstr  = (zbstring_t*)attribute->data.value;
            char*       string = (char*)malloc(zbstr->len + 1);
            memcpy(string, zbstr->data, zbstr->len);
            string[zbstr->len] = '\0';
            ESP_LOGI(TAG, "Peer Manufacturer is \"%s\"", string);
            free(string);
        }
        if (attribute->id == ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID &&
            attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_CHAR_STRING && attribute->data.value)
        {
            LOG_ERROR("Received manufacturer name");
            zbstring_t* zbstr  = (zbstring_t*)attribute->data.value;
            char*       string = (char*)malloc(zbstr->len + 1);
            memcpy(string, zbstr->data, zbstr->len);
            string[zbstr->len] = '\0';
            ESP_LOGI(TAG, "Peer Model is \"%s\"", string);
            free(string);
        }
    }
    return ESP_OK;
}

esp_err_t MillZigbee::readAttributeResponeHandler(const esp_zb_zcl_cmd_read_attr_resp_message_t* message)
{
    LOG_ERROR("Received attribute response handler");
    return ESP_OK;
}

esp_err_t MillZigbee::configureReportResponseHandler(const esp_zb_zcl_cmd_config_report_resp_message_t* message)
{
    LOG_ERROR("Received configure report handler");
    return ESP_OK;
}

esp_err_t MillZigbee::thermostatClusterMessageHandler(const esp_zb_zcl_thermostat_value_message_t* message)
{
    LOG_ERROR("Message received %d %d", message->heat_setpoint, message->mode);

    return ESP_OK;
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

esp_zb_ep_list_t* MillZigbee::createThermostatEndpoint(uint8_t endpoint_id, esp_zb_thermostat_cfg_t* thermostat)
{
    esp_zb_ep_list_t* endpointList = esp_zb_ep_list_create();

    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint           = endpoint_id,
        .app_profile_id     = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id      = ESP_ZB_HA_THERMOSTAT_DEVICE_ID,
        .app_device_version = 0};
    auto* pClusterList = createThermostatCluster(thermostat);
    esp_zb_ep_list_add_ep(endpointList, pClusterList, endpoint_config);
    return endpointList;
}

esp_zb_cluster_list_t* MillZigbee::createThermostatCluster(esp_zb_thermostat_cfg_t* thermostat)
{
    esp_zb_cluster_list_t*   pClusterList  = esp_zb_zcl_cluster_list_create();
    esp_zb_attribute_list_t* pBasicCluster = esp_zb_basic_cluster_create(&(thermostat->basic_cfg));
    esp_zb_basic_cluster_add_attr(
        pBasicCluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, const_cast<char*>(ESP_MANUFACTURER_NAME));
    esp_zb_basic_cluster_add_attr(
        pBasicCluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, const_cast<char*>(ESP_MODEL_IDENTIFIER));

    esp_zb_cluster_list_add_basic_cluster(pClusterList, pBasicCluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(
        pClusterList, esp_zb_identify_cluster_create(&(thermostat->identify_cfg)), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(
        pClusterList, esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
    esp_zb_cluster_list_add_thermostat_cluster(
        pClusterList, esp_zb_thermostat_cluster_create(&(thermostat->thermostat_cfg)), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    return pClusterList;
}
