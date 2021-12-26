/*
 *  modbus_api.c
 *
 *  Created on: Dec 26, 2021
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

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
} modbus_elec_info_t;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static const char* TAG = "MODBUS";
static uint32_t elec_slave_mask = MODBUS_ELEC_SLAVE_MASK;
// static uint32_t water_slave_mask = MODBUS_WATER_SLAVE_MASK;

const modbus_elec_info_t elec_reg_info[] = {
#define XTABLE_ITEM(id, name, type, address, size, flag) { id, address, size, flag, #name },
    MODBUS_ELEC_INPUT_REGS
#undef XTABLE_ITEM
};

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

static uint16_t modbus_api_get_num_reg(modbus_elec_reg_id start, modbus_elec_reg_id stop);
static void modbus_api_task(void *arg);

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
    const modbus_elec_reg_id start = MB_POWER_RECEIVE_WH, stop = MB_POWER_RECEIVE_WH;
    uint32_t i;
    uint8_t rx_buffer[MODBUS_RX_BUFFER_SIZE];
    bool result;

    uint16_t num_elec_reg = modbus_api_get_num_reg(start, stop);
    ESP_LOGI(TAG, "Number of electric meter registers %d", num_elec_reg); 
    while(1)
    {
        /* Read data from electric meter */
        for(i = 0; i < 32; i++)
        {
            if(((elec_slave_mask >> i) & 0x00000001) == 0x1)
            {
                /* Read from each slave */
                result = modbus_command_get_electric_registers(i, start, num_elec_reg, rx_buffer);
                if(result)
                {
                    ESP_LOGI(TAG, "Receive response from slave %d", i); 
                }
            }
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

/******************************************************************************/

/*!
 * @brief  Modbus in master mode initialization
 */
void modbus_api_init(void)
{
    /* Modbus command initialization */
    modbus_command_init();

    /* Create task for modbus get data */
    BaseType_t result = xTaskCreate(modbus_api_task, MODBUS_TASK_NAME, MODBUS_TASK_SIZE, NULL, MODBUS_TASK_PRIORITY, NULL);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Create modbus_api task fail %d", result);
        return;
    }
}