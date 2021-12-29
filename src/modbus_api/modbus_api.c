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

typedef void (*modbus_data_convert_t)(uint8_t*, char*);
const char *day_in_week[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat", "null"};
modbus_data_convert_t modbus_data_convert[MB_ELEC_NUMBER_OF_CMD];
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
                memcpy(&modbus_data.data[index], &buffer[12], modbus_reg_info[j].size);    /* Data index = 12 (see the document) */
                index += modbus_reg_info[j].size;
                vTaskDelay(MODBUS_RX_TIMEOUT_MS / portTICK_RATE_MS);    /* Delay before next*/
            }
            /* If read all register success, put to queue */
            if(result)
            {
                ESP_LOGI(TAG, "Receive response from slave"ADDRSTR, ADDR2STR(slave_address[i]));
                modbus_data.slave_id = i;
                modbus_api_queue_put(&modbus_data);
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
 * @brief  Add registers info to json array
 */
#ifdef ELECTRIC_METER_USED
/* year, month, day, day in week*/
void modbus_data_to_date(uint8_t *data, char* date_str)
{
    uint32_t year = data[3] + 2000;
    uint8_t i = (data[0] < 7) ? data[0] : 7;
    sprintf(date_str, "%s, %02d/%02d/%02d", day_in_week[i], data[2], data[1], year);
}

/* hh_mm_ss */
void modbus_data_to_time(uint8_t *data, char* date_str)
{
    sprintf(date_str, "%d:%d:%d", data[0], data[1], data[2]);
}

/* Total - Tariff 1 - Tariff 2 -Tariff 3 - Tariff 4 */
void modbus_data_to_energy(uint8_t *data, char* date_str)
{
    uint32_t *u32_value = (uint32_t*) data;
    sprintf(date_str, "total %u, tariff 1 %u, tariff 2 %u, tariff 3 %u, tariff 4 %u", u32_value[0], u32_value[1], u32_value[2], u32_value[3], u32_value[4]);
}

/* Unclear */
void modbus_data_to_day_table(uint8_t *data, char* date_str)
{
    sprintf(date_str, "loadding...");
}

/* Cycle - Null - Show bits */
void modbus_data_to_show_mode(uint8_t *data, char* date_str)
{
    sprintf(date_str, "cycle %d", data[0]);
}

/* Version - Type - Status */
void modbus_data_to_version(uint8_t *data, char* date_str)
{
    sprintf(date_str, "version %d, type %d, status %d", data[0], data[1], data[2]);
}

/* Meter constant */
void modbus_data_to_constant(uint8_t *data, char* date_str)
{
    sprintf(date_str, "constant %02X %02X %02X", data[0], data[1], data[2]);
}

/* Each byte was added MODBUS_DATA_ADD_BYTE */
void modbus_parse_raw_data(uint8_t *data, uint16_t length)
{
    for(uint16_t i = 0; i < length; i++)
    {
        data[i] = data[i] - MODBUS_DATA_ADD_BYTE;
    }
}

static void modbus_api_add_reg_data_to_json(cJSON* root, const modbus_reg_info_t *table, modbus_data_t *modbus_data)
{
    uint16_t i;
    char data_str[128];
    uint8_t *data = modbus_data->data;
    
    for(i = modbus_data->start; i <= modbus_data->stop; i++)
    {
        if(table[i].flag & REPORT)
        {
            cJSON* object = cJSON_CreateObject();
            modbus_parse_raw_data(data, table[i].size);
            modbus_data_convert[i](data, data_str);
            cJSON_AddStringToObject(object, JSON_NAME_KEY, table[i].name);
            cJSON_AddStringToObject(object, JSON_VALUE_KEY, data_str);
            cJSON_AddItemToArray(root, object);
        }
        /* Next data */
        data += table[i].size;
    }
}
#else
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
#endif

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
#ifdef ELECTRIC_METER_USED
    char slave_addr[32];
    sprintf(slave_addr, ADDRSTR, ADDR2STR(slave_address[modbus_data->slave_id]));
    cJSON_AddStringToObject(root, JSON_SLAVE_ID_KEY, slave_addr);
#else
    cJSON_AddNumberToObject(root, JSON_SLAVE_ID_KEY, modbus_data->slave_id);
#endif

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
 * @brief  Set slave info
 */
void modbus_api_set_slave(uint8_t *slave_id, uint8_t num_slave)
{
    slave_count = num_slave;
#ifdef ELECTRIC_METER_USED
    memcpy(slave_address, slave_id, slave_count * 6);
#else
    memcpy(slave_address, slave_id, slave_count);
#endif
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

#ifdef ELECTRIC_METER_USED
    modbus_data_convert[MB_DATE_CMD] = modbus_data_to_date;
    modbus_data_convert[MB_TIME_CMD] = modbus_data_to_time;
    modbus_data_convert[MB_ENERGY_CMD] = modbus_data_to_energy;
    modbus_data_convert[MB_SHOW_MODE_CMD] = modbus_data_to_show_mode;
    modbus_data_convert[MB_VERSION_CMD] = modbus_data_to_version;
    modbus_data_convert[MB_CONSTANT_CMD] = modbus_data_to_constant;
    modbus_data_convert[MB_DAY_TABLE_CMD] = modbus_data_to_day_table;
#endif
}