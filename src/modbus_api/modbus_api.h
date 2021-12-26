/*
 *  modbus_api.h
 *
 *  Created on: Dec 26, 2021
 */

#ifndef _MODBUS_API_H_
#define _MODBUS_API_H_

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <stdint.h>
#include <cJSON.h>
#include "modbus_table.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

typedef uint8_t meter_type_t;
enum {
    ELECTRIC_METER = 0,
    WATER_METER,
    METER_COUNT
};

typedef struct
{
    meter_type_t meter;
    uint8_t slave_id;
    modbus_elec_reg_id start;
    modbus_elec_reg_id stop;
    uint8_t data[MODBUS_COMMAND_MAX_SIZE];
} modbus_data_t;


/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/



/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

/*!
 * @brief  Get modbus data from queue
 * @param  [out] Data to store output
 * @retval ESP_OK if success
 *         ESP_FAIL if fail
 */
esp_err_t modbus_api_queue_get(modbus_data_t *modbus_data);

/*!
 * @brief  Put modbus data into queue
 * @param  [in] Data to put into queue
 * @retval ESP_OK if success
 *         ESP_FAIL if fail
 */
esp_err_t modbus_api_queue_put(modbus_data_t *modbus_data);

/*!
 * @brief  Modbus in master mode initialization
 * @param  None
 * @retval None
 */
void modbus_api_init(void);

/*!
 * @brief  Convert modbus data to json string
 * @param  None
 * @retval String response. NOTE: Must to free after use
 */
char* modbus_api_data_to_json(modbus_data_t *modbus_data);

/******************************************************************************/

#endif /* _MODBUS_API_H_ */