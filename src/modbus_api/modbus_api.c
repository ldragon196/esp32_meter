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
    modbus_reg_id id;
    uint16_t address;
    uint16_t size;
    uint8_t flag;
    const char *name;
} modbus_reg_info_t;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static const char* TAG = "MODBUS";
const char *meter_type[METER_COUNT] = {"electric", "water"};

static QueueHandle_t modbus_command_queue;
static uint32_t slave_count = MODBUS_SLAVE_COUNT;

#ifdef ELECTRIC_METER_USED
/* Electric meter*/
static uint8_t slave_address[MAX_SLAVE_ID][6] = MODBUS_SLAVE_ID_DEFAULT;
const modbus_reg_info_t modbus_reg_info[] = {
#define XTABLE_ITEM(id, name, type, address, size, flag) { id, address, size, flag, #name },
    MODBUS_ELECTRIC_CMD
#undef XTABLE_ITEM
};
#else
/* Water meter */
static uint8_t slave_address[MAX_SLAVE_ID] = MODBUS_SLAVE_ID_DEFAULT;
const modbus_reg_info_t modbus_reg_info[] = {
#define XTABLE_ITEM(id, name, type, address, size, flag) { id, address, size, flag, #name },
    MODBUS_WATER_INPUT_REGS
#undef XTABLE_ITEM
};
#endif

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

uint16_t modbus_api_get_num_reg(modbus_reg_id start, modbus_reg_id stop);
static void modbus_api_task(void *arg);
static void modbus_api_add_reg_data_to_json(cJSON* root, const modbus_reg_info_t *table, modbus_data_t *modbus_data);

/******************************************************************************/

/*!
 * @brief  Get number of register from "start" register to "stop" register
 */
uint16_t modbus_api_get_num_reg(modbus_reg_id start, modbus_reg_id stop)
{
    uint16_t ret_val = 0;
    for(modbus_reg_id i = start; i <= stop; i++)
    {
        ret_val += modbus_reg_info[i].size;
    }
    return ret_val;
}

/*!
 * @brief  Task for get data from slave
 */
#ifdef ELECTRIC_METER_USED
static void modbus_api_task(void *arg)
{
    uint32_t i, j, index;
    bool result;
    modbus_data_t modbus_data;
    uint8_t buffer[MODBUS_COMMAND_MAX_SIZE];

    while(1)
    {
        /* Read data from electric meter */
        modbus_data.meter = ELECTRIC_METER;
        modbus_data.start = MB_DATE_CMD;
        modbus_data.stop = MB_DATE_CMD;

        memset(&modbus_data, 0, sizeof(modbus_data));
        for(i = 0; i < slave_count; i++)
        {
            index = 0;    /* Index of data */
            result = false;
            for(j = modbus_data.start; j <= modbus_data.stop; j++)
            {
                /* Read data from start to stop */
                result = modbus_command_get_elec_registers(slave_address[i], modbus_reg_info[j].address, buffer, modbus_reg_info[j].size);
                if(!result)
                {
                    break;
                }
                memcpy(&modbus_data.data[index], buffer, modbus_reg_info[j].size);
                index += modbus_reg_info[j].size;
                vTaskDelay(100 / portTICK_RATE_MS);    /* Delay before next*/
            }
            /* If read all register success, put to queue */
            if(result)
            {
                ESP_LOGI(TAG, "Receive response from slave"ADDRSTR, ADDR2STR(slave_address[i]));
                modbus_data.slave_id = i;
                modbus_api_queue_put(&modbus_data);
                vTaskDelay(MODBUS_RX_TIMEOUT_MS / portTICK_RATE_MS);    /* Delay before next*/
            }
        }
        vTaskDelay(MODBUS_TIME_BETWEEN_POLLING_MS / portTICK_RATE_MS);
    }
}
#else
static void modbus_api_task(void *arg)
{
    uint32_t i;
    bool result;
    modbus_data_t modbus_data;
    uint16_t num_reg;
    
    while(1)
    {
        /* Read data from water meter */
        modbus_data.meter = WATER_METER;
        modbus_data.start = MB_POWER_RECEIVE_WH;
        modbus_data.stop = MB_POWER_RECEIVE_WH;
        num_reg = modbus_api_get_num_reg(modbus_data.start, modbus_data.stop);    /* Read from reg_1 to reg_2 */
        for(i = 0; i < slave_count; i++)
        {
            /* Read from each slave */
            result = modbus_command_get_water_registers(slave_address[i], modbus_reg_info[modbus_data.start].address, num_reg, modbus_data.data);
            if(result)
            {
                /* Put to queue */
                ESP_LOGI(TAG, "Receive response from slave %d", slave_address[i]);
                modbus_data.slave_id = slave_address[i];
                modbus_api_queue_put(&modbus_data);
                vTaskDelay(MODBUS_RX_TIMEOUT_MS / portTICK_RATE_MS);    /* Delay before next*/
            }
        }
        vTaskDelay(MODBUS_TIME_BETWEEN_POLLING_MS / portTICK_RATE_MS);
    }
}
#endif

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
    uint16_t i;
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
        modbus_api_add_reg_data_to_json(regs, modbus_reg_info, modbus_data);
    }

    /* Print json to string */
    char* ret_val = cJSON_Print(root);
    cJSON_Delete(root);
    return ret_val;
}