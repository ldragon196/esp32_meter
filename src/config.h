/*
 *  config.h
 *
 *  Created on: Dec 26, 2021
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

/* Info */
#define FIRMWARE_VERSION                              "1.0.0"
#define HARDWARE_VERSION                              "1.0.0"

/* Modbus */
#define MODBUS_RX_BUFFER_SIZE                         1024
#define MODBUS_RX_TIMEOUT_MS                          2000

#define MODBUS_ELEC_PORT_NUM                          UART_NUM_2
#define MODBUS_ELEC_BAUDRATE                          9600
#define MODBUS_ELEC_PARITY                            UART_PARITY_DISABLE
#define MODBUS_ELEC_UART_TXD                          23
#define MODBUS_ELEC_UART_RXD                          22

#define MODBUS_WATER_PORT_NUM                         UART_NUM_1
#define MODBUS_WATER_BAUDRATE                         1200
#define MODBUS_WATER_PARITY                           UART_PARITY_EVEN
#define MODBUS_WATER_UART_TXD                         18
#define MODBUS_WATER_UART_RXD                         17

#define MODBUS_ELEC_SLAVE_MASK                        0xFFFFFFFF
#define MODBUS_WATER_SLAVE_MASK                       0xFFFFFFFF

/* TASK */
#define MODBUS_TASK_NAME                              "modbus"
#define MODBUS_TASK_SIZE                              4096
#define MODBUS_TASK_PRIORITY                          3

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/



/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/



/******************************************************************************/

#endif /* _CONFIG_H_ */