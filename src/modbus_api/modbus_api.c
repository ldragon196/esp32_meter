/*
 *  modbus_api.c
 *
 *  Created on: Dec 26, 2021
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <cJSON.h>
#include "config.h"
#include "modbus_table.h"
#include "modbus_command.h"
#include "modbus_api.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

typedef struct {
    modbus_elec_reg_id id;
    uint16_t address;
    uint16_t size;
    uint8_t flag;
    const char *name;
} modbus_reg_info_t;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static const char* TAG = "MODBUS";
static uint32_t elec_slave_mask = MODBUS_ELEC_SLAVE_MASK;
// static uint32_t water_slave_mask = MODBUS_WATER_SLAVE_MASK;

static QueueHandle_t modbus_command_queue;

/* Electric meter registers info */
const modbus_reg_info_t elec_reg_info[] = {
#define XTABLE_ITEM(id, name, type, address, size, flag) { id, address, size, flag, #name },
    MODBUS_ELEC_INPUT_REGS
#undef XTABLE_ITEM
};

const char *meter_type[METER_COUNT] = {"electric", "water"};

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

static uint16_t modbus_api_get_num_reg(modbus_elec_reg_id start, modbus_elec_reg_id stop);
static void modbus_api_task(void *arg);
static void modbus_api_add_reg_data_to_json(cJSON* root, const modbus_reg_info_t *table, modbus_data_t *modbus_data);

/******************************************************************************/

/*!
 * @brief  Get number of register from "start" register to "stop" register
 */
static uint16_t modbus_api_get_num_reg(modbus_elec_reg_id start, modbus_elec_reg_id stop)
{
    uint16_t ret_val = 0;
    for(modbus_elec_reg_id i = start; i <= stop; i++)
    {
        ret_val += elec_reg_info[i].size;
    }
    return ret_val;
}

/*!
 * @brief  Task for get data from slave
 */
static void modbus_api_task(void *arg)
{
    uint32_t i;
    bool result;
    modbus_data_t modbus_data;
    uint16_t num_elec_reg;
    
    while(1)
    {
        /* Read data from electric meter */
        modbus_data.meter = ELECTRIC_METER;
        modbus_data.start = MB_POWER_RECEIVE_WH;
        modbus_data.stop = MB_POWER_TRANSMISS_WH;
        num_elec_reg = modbus_api_get_num_reg(modbus_data.start, modbus_data.stop);    /* Read from reg_1 to reg_2 */
        for(i = 0; i < 32; i++)
        {
            if(((elec_slave_mask >> i) & 0x00000001) == 0x1)
            {
                /* Read from each slave */
                result = modbus_command_get_electric_registers(i, elec_reg_info[modbus_data.start].address, num_elec_reg, modbus_data.data);
                if(result)
                {
                    /* Put to queue */
                    ESP_LOGI(TAG, "Receive response from slave %d", i);
                    modbus_data.slave_id = i;
                    modbus_api_queue_put(&modbus_data);
                    vTaskDelay(MODBUS_RX_TIMEOUT_MS / portTICK_RATE_MS);    /* Delay before next*/
                }
            }
        }
        vTaskDelay(MODBUS_TIME_BETWEEN_POLLING_MS / portTICK_RATE_MS);
    }
}

/******************************************************************************/

/*!
 * @brief  Get modbus data from queue
 */
esp_err_t modbus_api_queue_get(modbus_data_t *modbus_data)
{
    BaseType_t result = xQueueReceive(modbus_command_queue, modbus_data, pdMS_TO_TICKS(MODBUS_QUEUE_TIMEOUT_MS));
    return (result == pdPASS) ? ESP_OK : ESP_FAIL;
}

/*!
 * @brief  Put modbus data into queue
 */
esp_err_t modbus_api_queue_put(modbus_data_t *modbus_data)
{
    if(modbus_command_queue != NULL)
    {
        BaseType_t result = xQueueSend(modbus_command_queue, modbus_data, pdMS_TO_TICKS(MODBUS_QUEUE_TIMEOUT_MS));
        if(result == pdPASS)
        {
            return ESP_OK;
        }
        /* Drop oldest package */
        if(result == errQUEUE_FULL)
        {
            modbus_data_t tmp;
            ESP_LOGI(TAG, "Queue is full. Drop oldest package");
            xQueueReceive(modbus_command_queue, &tmp, pdMS_TO_TICKS(MODBUS_QUEUE_TIMEOUT_MS));
            /* Put again */
            result = xQueueSend(modbus_command_queue, modbus_data, pdMS_TO_TICKS(MODBUS_QUEUE_TIMEOUT_MS));
            if(result == pdPASS)
            {
                return ESP_OK;
            }
        }
    }
    return ESP_FAIL;
}

/*!
 * @brief  Modbus in master mode initialization
 */
void modbus_api_init(void)
{
    /* Modbus command initialization */
    modbus_command_init();

    /* Creat modbus command queue */
    modbus_command_queue = xQueueCreate(MODBUS_QUEUE_SIZE, sizeof(modbus_data_t));
    if(modbus_command_queue == NULL)
    {
        ESP_LOGE(TAG, "Create modbus queue fail");
        return;
    }

    /* Create task for modbus get data */
    BaseType_t result = xTaskCreate(modbus_api_task, MODBUS_TASK_NAME, MODBUS_TASK_SIZE, NULL, MODBUS_TASK_PRIORITY, NULL);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Create modbus_api task fail %d", result);
    }
}

/******************************************************************************/

/*!
 * @brief  Add registers info to json array
 */
static void modbus_api_add_reg_data_to_json(cJSON* root, const modbus_reg_info_t *table, modbus_data_t *modbus_data)
{
    modbus_elec_reg_id i;
    uint32_t *value = (uint32_t*) &modbus_data->data[3];    /* Packet: slave_id - function id - byte count - data0 - data1... */
    char addr_str[8];

    for(i = modbus_data->start; i <= modbus_data->stop; i++)
    {
        if(table[i].flag & REPORT)
        {
            cJSON* object = cJSON_CreateObject();
            cJSON_AddStringToObject(object, JSON_NAME_KEY, table[i].name);
            sprintf(addr_str, "0x%04X", table[i].address);
            cJSON_AddStringToObject(object, JSON_ADDRESS_KEY, addr_str);
            cJSON_AddNumberToObject(object, JSON_VALUE_KEY, *value++);
            cJSON_AddItemToArray(root, object);
        }
    }
}

/*!
 * @brief  Convert modbus data to json string
 */
char* modbus_api_data_to_json(modbus_data_t *modbus_data)
{
    cJSON* root = cJSON_CreateObject();
    if(root == NULL)
    {
        ESP_LOGE(TAG, "Cannot create json root");
        return NULL;
    }
    
    /* Add item to root */
    cJSON_AddStringToObject(root, JSON_METER_TYPE_KEY, meter_type[modbus_data->meter]);
    cJSON_AddNumberToObject(root, JSON_SLAVE_ID_KEY, modbus_data->slave_id);
    cJSON* regs = cJSON_AddArrayToObject(root, JSON_REG_KEY);
    if(regs != NULL)
    {
        if(modbus_data->meter == ELECTRIC_METER)
        {
            modbus_api_add_reg_data_to_json(regs, elec_reg_info, modbus_data);
        }
    }

    /* Print json to string */
    char* ret_val = cJSON_Print(root);
    cJSON_Delete(root);
    return ret_val;
}