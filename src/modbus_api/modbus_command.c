/*
 *  modbus_command.c
 *
 *  Created on: Dec 26, 2021
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <driver/uart.h>
#include "config.h"
#include "utility/utility.h"
#include "modbus_command.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define FAIL_CHECK(a, str, ...)                                                      \
    if (!(a)) {                                                                      \
        ESP_LOGE(TAG, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__);        \
        return false;                                                                \
    }

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static const char* TAG = "COMMAND";

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

static bool modbus_command_transceiver(uart_port_t uart_port, uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t max_size, uint16_t *rx_size);

/******************************************************************************/

/*!
 * @brief  UART transceiver
 */
static bool modbus_command_transceiver(uart_port_t uart_port, uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t max_size, uint16_t *rx_size)
{
    *rx_size = 0;
    
    /* Flush uart buffer before */
    uart_flush(uart_port);

    /* Write data to the UART */
    int rc = uart_write_bytes(uart_port, (const char*)tx_data, tx_size);
    uart_wait_tx_done(uart_port, -1);
    FAIL_CHECK((rc > 0), "Cannot write uart port %d", uart_port);

    /* Read data from UART */
    rc = uart_read_bytes(uart_port, rx_data, max_size, pdMS_TO_TICKS(MODBUS_RX_TIMEOUT_MS));
    FAIL_CHECK((rc > 0), "No response");

    *rx_size = rc;
    return true;
}

/******************************************************************************/

/*!
 * @brief  Get water meter registers
 */
bool modbus_command_get_water_registers(uint8_t slave_id, uint16_t address, uint16_t num_reg, uint8_t *rx_data)
{
    uint8_t command[8];
    uint16_t crc16, crc16_receive;
    uint16_t rx_size = 0;

    /* Build command */
    command[0] = slave_id;
    command[1] = MODBUS_READ_INPUT_FUNCTION;
    command[2] = HI_UINT16(address);
    command[3] = LO_UINT16(address);
    command[4] = HI_UINT16(num_reg);
    command[5] = LO_UINT16(num_reg);
    crc16 = crc16_modbus(command, 6);
    command[6] = HI_UINT16(crc16);
    command[7] = LO_UINT16(crc16);
    
    /* Send and receive data */
    uint8_t max_size = RAW_LEN(num_reg) + 5;    /* 1 Address + 1 Function + 1 Byte count + 2 CRC */
    if(modbus_command_transceiver(MODBUS_PORT_NUM, command, 8, rx_data, max_size, &rx_size))
    {
        /* Check valid */
        if(rx_size == max_size)
        {
            crc16 = crc16_modbus(rx_data, rx_size - 2);
            crc16_receive = MERGE_UINT16(rx_data[rx_size - 2], rx_data[rx_size - 1]);
            FAIL_CHECK((crc16 == crc16_receive), "Slave %d CRC error, %d != %d", slave_id, crc16, crc16_receive);
            return true;
        }
    }
    FAIL_CHECK(0, "Slave %d no response", slave_id);
    return false;
}

/*!
 * @brief  Get electric meter registers
 */
bool modbus_command_get_elec_registers(uint8_t *slave_id, uint16_t address, uint8_t *rx_data, uint8_t rx_data_size)
{
    uint8_t command[14];
    uint8_t sum;
    uint16_t rx_size, max_size;

    /* Build command */
    command[0] = MODBUS_START_BYTE;
    memcpy(&command[1], slave_id, 6);
    command[7] = MODBUS_START_BYTE;
    command[8] = MODBUS_READ_REQUEST_BYTE;
    command[9] = 2;    /* Length */
    command[10] = HI_UINT16(address);
    command[11] = LO_UINT16(address);
    command[12] = check_sum(command, 12);
    command[13] = MODBUS_END_BYTE;

    /* Send and receive data */
    max_size = 14 + rx_data_size;
    if(modbus_command_transceiver(MODBUS_PORT_NUM, command, 14, rx_data, max_size, &rx_size))
    {
        /* Check valid */
        if((rx_size == max_size) && (rx_data[0] == MODBUS_START_BYTE) && (rx_data[rx_size - 1] == MODBUS_END_BYTE))
        {
            sum = check_sum(rx_data, rx_size - 2);
            FAIL_CHECK((sum == rx_data[rx_size - 2]), "Check sum error from slave"ADDRSTR, ADDR2STR(slave_id));
            return true;
        }
        else
        {
            FAIL_CHECK(0, "Invalid data from slave "ADDRSTR, ADDR2STR(slave_id));
        }
    }
    FAIL_CHECK(0, "No response from slave "ADDRSTR, ADDR2STR(slave_id));
    return false;
}

/******************************************************************************/

/*!
 * @brief  UART for modbus initialization
 */
void modbus_command_init(void)
{
    /* Configure parameters of an UART driver, communication pins and install the driver */
    uart_config_t uart_config = {
            .baud_rate = MODBUS_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity    = MODBUS_PARITY,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
    };
    
    /* Init UART for modbus master */
    ESP_ERROR_CHECK(uart_driver_install(MODBUS_PORT_NUM, MODBUS_RX_BUFFER_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(MODBUS_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(MODBUS_PORT_NUM, MODBUS_UART_TXD, MODBUS_UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}