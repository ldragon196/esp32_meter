/*
 *  modbus_command.h
 *
 *  Created on: Dec 26, 2021
 */

#ifndef _MODBUS_COMMAND_H_
#define _MODBUS_COMMAND_H_

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/



/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

/*  */
#define MODBUS_READ_INPUT_FUNCTION                    0x04
#define MODBUS_WRITE_REGISTER_FUNCTION                0x06

#define REG_LEN(raw_len)                              ((raw_len) >> 1)
#define RAW_LEN(reg_len)                              ((reg_len) << 1)

/*  */
#define MODBUS_START_BYTE                             0x68
#define MODBUS_END_BYTE                               0x16

#define MODBUS_READ_REQUEST_BYTE                      0x01
#define MODBUS_READ_RESPONSE_BYTE                     0x81
#define MODBUS_WRITE_REQUEST_BYTE                     0x04
#define MODBUS_WRITE_RESPONSE_BYTE                    0x84

#define ADDRSTR                                       "%02X %02X %02X %02X %02X %02X"
#define get_byte(data, idx)                           (((const uint8_t*)(data))[idx])
#define ADDR2STR(addr)                                get_byte(addr,0), \
                                                      get_byte(addr,1), \
                                                      get_byte(addr,2), \
                                                      get_byte(addr,3), \
                                                      get_byte(addr,4), \
                                                      get_byte(addr,5) \

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
 * @brief  Get water meter registers
 * @param  Slave id
 *         Register address
 *         Number of registers
 *         Rx data
 * @retval True if success
 */
bool modbus_command_get_water_registers(uint8_t slave_id, uint16_t address, uint16_t num_reg, uint8_t *rx_data);

/*!
 * @brief  Get electric meter registers
 * @param  Slave address
 *         Register address
 *         Rx data
 *         Rx Data size
 * @retval True if success
 */
bool modbus_command_get_elec_registers(uint8_t *slave_id, uint16_t address, uint8_t *rx_data, uint8_t rx_data_size);

/*!
 * @brief  UART for modbus initialization
 * @param  None
 * @retval None
 */
void modbus_command_init(void);

/******************************************************************************/

#endif /* _MODBUS_COMMAND_H_ */