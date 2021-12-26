/*
 *  main.c
 *
 *  Created on: Dec 26, 2021
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <nvs_flash.h>
#include "config.h"
#include "modbus_api/modbus_api.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/



/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static const char* TAG = "MAIN";

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/



/******************************************************************************/

/**
 * @brief  Main app
 */
void app_main()
{
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Power up! Firmware version %s, hardware version %s", FIRMWARE_VERSION, HARDWARE_VERSION);
    
    /* Modbus master init */
    modbus_api_init();

    modbus_data_t modbus_data;
    while(1)
    {
        if(modbus_api_queue_get(&modbus_data) == ESP_OK)
        {
            char *message = modbus_api_data_to_json(&modbus_data);
            if(message != NULL)
            {
                ESP_LOGI(TAG, "-------------- %s", message);
                free(message);
            }
        }
        ESP_LOGI(TAG, "Free heap %u", esp_get_minimum_free_heap_size());
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}