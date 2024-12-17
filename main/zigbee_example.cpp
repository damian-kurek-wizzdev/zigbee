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
#include "defines.h"
#include "sleep.h"
#include "zb_config_platform.h"
#include "zcl/esp_zigbee_zcl_command.h"
#include "zcl/esp_zigbee_zcl_commissioning.h"
#include "zcl/esp_zigbee_zcl_thermostat.h"
static const char* LOG_TAG = "Zigbee_mill";
#define LOG_AL_LEVEL ESP_LOG_INFO

#include "esp_zigbee_core.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"
#include "string.h"
#include "zcl/esp_zigbee_zcl_common.h"
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
constexpr const char* ESP_MANUFACTURER_NAME = "Espressif";    /* Customized manufacturer name */
constexpr const char* ESP_MODEL_IDENTIFIER  = "Mill zigbee "; /* Customized model identifier */

constexpr const char*    TAG                           = "ESP_HA_ON_OFF_SWITCH";
constexpr const uint8_t  OPERATION_MODE_HEATING_ONLY   = 2;
constexpr const uint8_t  SYSTEM_MODE_HEATING           = 4;
constexpr const uint8_t  SYSTEM_MODE_AUTO              = 1;
constexpr const uint8_t  SYSTEM_MODE_OFF               = 0;
constexpr const uint16_t ZIGBEE_TEMPERATURE_MULTIPLIER = 100; // We need to multiply value by 100 when
constexpr const uint32_t ZIGBEE_USE_ALL_CHANNELS       = 0x07FFF800;

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
    ESP_RETURN_ON_FALSE(
        esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK,
        ,
        TAG,
        "Failed to start Zigbee bdb commissioning");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct)
{
    uint32_t*                                p_sg_p           = signal_struct->p_app_signal;
    esp_err_t                                err_status       = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t                 sig_type         = static_cast<esp_zb_app_signal_type_t>(*p_sg_p);
    esp_zb_zdo_signal_device_annce_params_t* dev_annce_params = NULL;
    switch (sig_type)
    {
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            LOG_ERROR("TUTAJ");
            ESP_LOGI(TAG, "Initialize Zigbee stack");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
            break;
        case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
            if (err_status == ESP_OK)
            {
                ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
                if (esp_zb_bdb_is_factory_new())
                {
                    ESP_LOGI(TAG, "Start network formation");
                    LOG_ERROR("TUTA324434J");
                    // esp_zb_bdb_open_network(180);

                    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
                }
                else
                {
                    esp_zb_bdb_open_network(180);
                    ESP_LOGI(TAG, "Device rebooted");
                }
            }
            else
            {
                ESP_LOGE(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
            }
            break;
        case ESP_ZB_BDB_SIGNAL_FORMATION:
            if (err_status == ESP_OK)
            {
                esp_zb_ieee_addr_t extended_pan_id;
                esp_zb_get_extended_pan_id(extended_pan_id);
                LOG_ERROR("Signal formation");

                ESP_LOGI(
                    TAG,
                    "Formed network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: "
                    "0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                    extended_pan_id[7],
                    extended_pan_id[6],
                    extended_pan_id[5],
                    extended_pan_id[4],
                    extended_pan_id[3],
                    extended_pan_id[2],
                    extended_pan_id[1],
                    extended_pan_id[0],
                    esp_zb_get_pan_id(),
                    esp_zb_get_current_channel(),
                    esp_zb_get_short_address());
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);

                // bdb_start_top_level_commissioning_cb()
            }
            else
            {
                LOG_ERROR("sTART TOP LEVEL COMMISIONING");
                ESP_LOGI(TAG, "Restart network formation (status: %s)", esp_err_to_name(err_status));
                esp_zb_scheduler_alarm(
                    (esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_FORMATION, 1000);
            }
            break;
        case ESP_ZB_BDB_SIGNAL_STEERING:
            if (err_status == ESP_OK)
            {
                ESP_LOGI(TAG, "Network steering started");
            }
            break;
        case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE:
            dev_annce_params = (esp_zb_zdo_signal_device_annce_params_t*)esp_zb_app_signal_get_params(p_sg_p);
            ESP_LOGI(TAG, "New device commissioned or rejoined (short: 0x%04hx)", dev_annce_params->device_short_addr);
            esp_zb_zdo_match_desc_req_param_t cmd_req;
            cmd_req.dst_nwk_addr     = dev_annce_params->device_short_addr;
            cmd_req.addr_of_interest = dev_annce_params->device_short_addr;
            break;
        case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS:
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

static void esp_app_zb_attribute_handler(uint16_t cluster_id, const esp_zb_zcl_attribute_t* attribute)
{
    LOG_ERROR("ATTR handler");
    /* Basic cluster attributes */
    if (cluster_id == ESP_ZB_ZCL_CLUSTER_ID_BASIC)
    {
        if (attribute->id == ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID &&
            attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_CHAR_STRING && attribute->data.value)
        {
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
            zbstring_t* zbstr  = (zbstring_t*)attribute->data.value;
            char*       string = (char*)malloc(zbstr->len + 1);
            memcpy(string, zbstr->data, zbstr->len);
            string[zbstr->len] = '\0';
            ESP_LOGI(TAG, "Peer Model is \"%s\"", string);
            free(string);
        }
    }

    /* Temperature Measurement cluster attributes */
    if (cluster_id == ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT)
    {
        if (attribute->id == ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID &&
            attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_S16)
        {
            int16_t value = attribute->data.value ? *(int16_t*)attribute->data.value : 0;
            ESP_LOGI(TAG, "Measured Value is %.2f degrees Celsius", zb_s16_to_temperature(value));
        }
        if (attribute->id == ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_MIN_VALUE_ID &&
            attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_S16)
        {
            int16_t min_value = attribute->data.value ? *(int16_t*)attribute->data.value : 0;
            ESP_LOGI(TAG, "Min Measured Value is %.2f degrees Celsius", zb_s16_to_temperature(min_value));
        }
        if (attribute->id == ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_MAX_VALUE_ID &&
            attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_S16)
        {
            int16_t max_value = attribute->data.value ? *(int16_t*)attribute->data.value : 0;
            ESP_LOGI(TAG, "Max Measured Value is %.2f degrees Celsius", zb_s16_to_temperature(max_value));
        }
        if (attribute->id == ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_ID &&
            attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_U16)
        {
            uint16_t tolerance = attribute->data.value ? *(uint16_t*)attribute->data.value : 0;
            ESP_LOGI(TAG, "Tolerance is %.2f degrees Celsius", 1.0 * tolerance / 100);
        }
    }
}

static esp_err_t zb_attribute_reporting_handler(const esp_zb_zcl_report_attr_message_t* message)
{
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
    esp_app_zb_attribute_handler(message->cluster, &message->attribute);
    return ESP_OK;
}

static esp_err_t zb_read_attr_resp_handler(const esp_zb_zcl_cmd_read_attr_resp_message_t* message)
{
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(
        message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS,
        ESP_ERR_INVALID_ARG,
        TAG,
        "Received message: error status(%d)",
        message->info.status);

    ESP_LOGI(
        TAG,
        "Read attribute response: from address(0x%x) src endpoint(%d) to dst endpoint(%d) cluster(0x%x)",
        message->info.src_address.u.short_addr,
        message->info.src_endpoint,
        message->info.dst_endpoint,
        message->info.cluster);

    esp_zb_zcl_read_attr_resp_variable_t* variable = message->variables;
    while (variable)
    {
        ESP_LOGI(
            TAG,
            "Read attribute response: status(%d), cluster(0x%x), attribute(0x%x), type(0x%x), value(%d)",
            variable->status,
            message->info.cluster,
            variable->attribute.id,
            variable->attribute.data.type,
            variable->attribute.data.value ? *(uint8_t*)variable->attribute.data.value : 0);
        if (variable->status == ESP_ZB_ZCL_STATUS_SUCCESS)
        {
            esp_app_zb_attribute_handler(message->info.cluster, &variable->attribute);
        }

        variable = variable->next;
    }

    return ESP_OK;
}

static esp_err_t zb_configure_report_resp_handler(const esp_zb_zcl_cmd_config_report_resp_message_t* message)
{
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(
        message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS,
        ESP_ERR_INVALID_ARG,
        TAG,
        "Received message: error status(%d)",
        message->info.status);

    esp_zb_zcl_config_report_resp_variable_t* variable = message->variables;
    while (variable)
    {
        ESP_LOGI(
            TAG,
            "Configure report response: status(%d), cluster(0x%x), direction(0x%x), attribute(0x%x)",
            variable->status,
            message->info.cluster,
            variable->direction,
            variable->attribute_id);
        variable = variable->next;
    }

    return ESP_OK;
}

static esp_err_t zb_thermo_core(const esp_zb_zcl_thermostat_value_message_t* message)
{
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(
        message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS,
        ESP_ERR_INVALID_ARG,
        TAG,
        "Received message: error status(%d)",
        message->info.status);

    LOG_ERROR("Message received %d %d", message->heat_setpoint, message->mode);

    return ESP_OK;
}

static esp_err_t zb_thermo_weekly(const esp_zb_zcl_thermostat_weekly_schedule_set_message_t* message)
{
    LOG_ERROR(
        "Message received %d %d %d %lu",
        message->day_of_week,
        message->mode_for_req,
        message->trans.transition_time,
        message->trans_status);

    return ESP_OK;
}

static esp_err_t zbSetAttrbibuteCallback(const esp_zb_zcl_set_attr_value_message_t* pMessage)
{
    LOG_ERROR("Attribute received callback %u", pMessage->attribute.id);
    if (pMessage->attribute.id == ESP_ZB_ZCL_ATTR_THERMOSTAT_SYSTEM_MODE_ID)
    {
        LOG_ERROR("System mode");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void* message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id)
    {
        case ESP_ZB_CORE_REPORT_ATTR_CB_ID:
            LOG_ERROR("Attrubyte report handler");
            ret = zb_attribute_reporting_handler((esp_zb_zcl_report_attr_message_t*)message);
            break;
        case ESP_ZB_CORE_CMD_READ_ATTR_RESP_CB_ID:
            LOG_ERROR("Read attr response");
            ret = zb_read_attr_resp_handler((esp_zb_zcl_cmd_read_attr_resp_message_t*)message);
            break;
        case ESP_ZB_CORE_CMD_REPORT_CONFIG_RESP_CB_ID:
            LOG_ERROR("Configure report respons handler");
            ret = zb_configure_report_resp_handler((esp_zb_zcl_cmd_config_report_resp_message_t*)message);
            break;
        case ESP_ZB_CORE_THERMOSTAT_VALUE_CB_ID:
            LOG_ERROR("Theremo cb");
            ret = zb_thermo_core((esp_zb_zcl_thermostat_value_message_t*)message);
            break;
        case ESP_ZB_CORE_THERMOSTAT_WEEKLY_SCHEDULE_SET_CB_ID:
            ret = zb_thermo_weekly((esp_zb_zcl_thermostat_weekly_schedule_set_message_t*)message);
            LOG_ERROR("Theremo WEEKLY");
            break;

        case ESP_ZB_CORE_CMD_THERMOSTAT_GET_WEEKLY_SCHEDULE_RESP_CB_ID:
            LOG_ERROR("Theremo get WEEKLY");
            break;

        case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
            ret = zbSetAttrbibuteCallback((esp_zb_zcl_set_attr_value_message_t*)message);
            LOG_ERROR("SET ATTRIBUTE");

        default:
            ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
            break;
    }
    LOG_ERROR("Return value = %d", ret);
    return ret;
}

static esp_zb_cluster_list_t* custom_thermostat_clusters_create(esp_zb_thermostat_cfg_t* thermostat)
{
    esp_zb_cluster_list_t*   cluster_list  = esp_zb_zcl_cluster_list_create();
    esp_zb_attribute_list_t* basic_cluster = esp_zb_basic_cluster_create(&(thermostat->basic_cfg));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(
        basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, const_cast<char*>(ESP_MANUFACTURER_NAME)));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(
        basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, const_cast<char*>(ESP_MODEL_IDENTIFIER)));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(cluster_list, basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(
        cluster_list, esp_zb_identify_cluster_create(&(thermostat->identify_cfg)), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(
        cluster_list, esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_thermostat_cluster(
        cluster_list, esp_zb_thermostat_cluster_create(&(thermostat->thermostat_cfg)), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    /* Add temperature measurement cluster for attribute reporting */
    // ESP_ERROR_CHECK(esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list,
    // esp_zb_temperature_meas_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));
    return cluster_list;
}

static esp_zb_ep_list_t* custom_thermostat_ep_create(uint8_t endpoint_id, esp_zb_thermostat_cfg_t* thermostat)
{
    thermostat->thermostat_cfg.control_sequence_of_operation = OPERATION_MODE_HEATING_ONLY;
    thermostat->thermostat_cfg.occupied_heating_setpoint     = 25;
    thermostat->thermostat_cfg.system_mode                   = SYSTEM_MODE_HEATING;
    thermostat->thermostat_cfg.local_temperature             = 20;
    esp_zb_ep_list_t* ep_list                                = esp_zb_ep_list_create();

    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint           = endpoint_id,
        .app_profile_id     = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id      = ESP_ZB_HA_THERMOSTAT_DEVICE_ID,
        .app_device_version = 0};
    esp_zb_ep_list_add_ep(ep_list, custom_thermostat_clusters_create(thermostat), endpoint_config);
    return ep_list;
}

void scanCallback(esp_zb_zdp_status_t zdo_status, uint8_t count, esp_zb_network_descriptor_t* nwk_descriptor)
{
    LOG_ERROR("Scan done %d", count);
}

void esp_zb_task(void* pvParameters)
{
    /* Initialize Zigbee stack */
    esp_zb_cfg_t zb_nwk_cfg               = {};
    zb_nwk_cfg.esp_zb_role                = ESP_ZB_DEVICE_TYPE_ED;
    zb_nwk_cfg.install_code_policy        = INSTALLCODE_POLICY_ENABLE;
    zb_nwk_cfg.nwk_cfg.zed_cfg.ed_timeout = ED_AGING_TIMEOUT;
    zb_nwk_cfg.nwk_cfg.zed_cfg.keep_alive = ED_KEEP_ALIVE;

    esp_zb_init(&zb_nwk_cfg);

    /* Create customized thermostat endpoint */
    esp_zb_thermostat_cfg_t thermostat_cfg       = ESP_ZB_DEFAULT_THERMOSTAT_CONFIG();
    esp_zb_ep_list_t*       esp_zb_thermostat_ep = custom_thermostat_ep_create(HA_THERMOSTAT_ENDPOINT, &thermostat_cfg);
    /* Register the device */
    auto result = esp_zb_device_register(esp_zb_thermostat_ep);
    LOG_ERROR("Register %d", result);

    esp_zb_core_action_handler_register(zb_action_handler);
    result = esp_zb_set_primary_network_channel_set(0x07FFF800);
    LOG_ERROR("Channel %d", result);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_secur_network_min_join_lqi_set(1);
    // esp_zb_zcl_commissioning_init_server();

    esp_zb_stack_main_loop();
}

void runZigbee()
{
    esp_zb_task(nullptr);
}

void commisionTask(void* pvParameters)
{
    SLEEP_MS(1000);
    // esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
    // esp_zb_bdb_open_network(180);

    SLEEP_MS(3000 * 3000);
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

esp_err_t MillZigbee::zigbeeActionHandlerStatic(esp_zb_core_action_callback_id_t callbackId, const void* message)
{
    return pInstance->zigbeeActionHandler(callbackId, message);
}

esp_err_t MillZigbee::zigbeeActionHandler(esp_zb_core_action_callback_id_t callbackId, const void* message)
{
    esp_err_t ret = ESP_OK;
    switch (callbackId)
    {
        case ESP_ZB_CORE_REPORT_ATTR_CB_ID:
            LOG_ERROR("Attrubyte report handler");
            ret = zb_attribute_reporting_handler((esp_zb_zcl_report_attr_message_t*)message);
            break;
        case ESP_ZB_CORE_CMD_READ_ATTR_RESP_CB_ID:
            LOG_ERROR("Read attr response");
            ret = zb_read_attr_resp_handler((esp_zb_zcl_cmd_read_attr_resp_message_t*)message);
            break;
        case ESP_ZB_CORE_CMD_REPORT_CONFIG_RESP_CB_ID:
            LOG_ERROR("Configure report respons handler");
            ret = zb_configure_report_resp_handler((esp_zb_zcl_cmd_config_report_resp_message_t*)message);
            break;
        case ESP_ZB_CORE_THERMOSTAT_VALUE_CB_ID:
            LOG_ERROR("Theremo cb");
            ret = zb_thermo_core((esp_zb_zcl_thermostat_value_message_t*)message);
            break;
        case ESP_ZB_CORE_THERMOSTAT_WEEKLY_SCHEDULE_SET_CB_ID:
            ret = zb_thermo_weekly((esp_zb_zcl_thermostat_weekly_schedule_set_message_t*)message);
            LOG_ERROR("Theremo WEEKLY");
            break;

        case ESP_ZB_CORE_CMD_THERMOSTAT_GET_WEEKLY_SCHEDULE_RESP_CB_ID:
            LOG_ERROR("Theremo get WEEKLY");
            break;

        case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
            ret = setAttrbibuteCallback((esp_zb_zcl_set_attr_value_message_t*)message);
            LOG_ERROR("SET ATTRIBUTE");

        default:
            LOG_ERROR("Unkown callback received %d", callbackId);
            break;
    }
    LOG_ERROR("Return value = %d", ret);
    return ret;
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

    esp_zb_ep_list_t* pThermostatEndpoint = custom_thermostat_ep_create(HA_THERMOSTAT_ENDPOINT, &thermostatConfig);
    /* Register the device */
    auto result = esp_zb_device_register(pThermostatEndpoint);
    LOG_ERROR("Register %d", result);

    esp_zb_core_action_handler_register(zb_action_handler);
    result = esp_zb_set_primary_network_channel_set(ZIGBEE_USE_ALL_CHANNELS);
    LOG_ERROR("Channel %d", result);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_secur_network_min_join_lqi_set(1);
    // esp_zb_zcl_commissioning_init_server();

    esp_zb_stack_main_loop();
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

void MillZigbee::startZigbee()
{
    auto result = xTaskCreate(esp_zb_task, "Zigbee_main", 4096, this, 5, &m_taskHandle);
    if (result != pdPASS)
        LOG_ERROR("Failed to create a task: %s", "Zigbee_main");
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
