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

#define MODBUS_READ_INPUT_FUNCTION                    0x04
#define MODBUS_WRITE_REGISTER_FUNCTION                0x06

#define REG_LEN(raw_len)                              ((raw_len) >> 1)
#define RAW_LEN(reg_len)                              ((reg_len) << 1)

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
 * @brief  Get electric meter registers
 * @param  Slave id
 *         Register address
 *         Number of registers
 *         Rx data
 * @retval True if success
 */
bool modbus_command_get_electric_registers(uint8_t slave_id, uint16_t address, uint16_t num_reg, uint8_t *rx_data);

/*!
 * @brief  UART for modbus initialization
 * @param  None
 * @retval None
 */
void modbus_command_init(void);

/******************************************************************************/

#endif /* _MODBUS_COMMAND_H_ */