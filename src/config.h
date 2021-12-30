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
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#ifndef ELECTRIC_METER_USED
#define ELECTRIC_METER_USED
#endif

/* Info */
#define FIRMWARE_VERSION                              "1.0.0"
#define HARDWARE_VERSION                              "1.0.0"

/* Wifi config */
#define WIFI_SSID                                     "A501"
#define WIFI_PASSWORD                                 "1133557799"
#define WIFI_SSID_MAX_LENGTH                          32
#define WIFI_PASSWORD_MAX_LENGTH                      64
#define WIFI_TIME_RETRY_CONNECT_MS                    3000

/* Modbus */
#define MAX_SLAVE_ID                                  32
#define MODBUS_RX_BUFFER_SIZE                         1024
#define MODBUS_COMMAND_MAX_SIZE                       128
#define MODBUS_QUEUE_SIZE                             128
#define MODBUS_QUEUE_TIMEOUT_MS                       50

#define MODBUS_TIME_BETWEEN_POLLING_MS                5000
#define MODBUS_RX_TIMEOUT_MS                          1000

#ifdef ELECTRIC_METER_USED
#define MODBUS_PORT_NUM                               UART_NUM_2
#define MODBUS_BAUDRATE                               1200
#define MODBUS_PARITY                                 UART_PARITY_EVEN
#define MODBUS_UART_TXD                               23
#define MODBUS_UART_RXD                               22
#define MODBUS_SLAVE_COUNT                            2
#define MODBUS_SLAVE_ID_DEFAULT                       {{1, 2, 3, 4, 5, 6}, {11, 12, 13, 14, 15, 16}, }
#else
#define MODBUS_PORT_NUM                               UART_NUM_2
#define MODBUS_BAUDRATE                               9600
#define MODBUS_PARITY                                 UART_PARITY_DISABLE
#define MODBUS_UART_TXD                               23
#define MODBUS_UART_RXD                               22
#define MODBUS_SLAVE_COUNT                            2
#define MODBUS_SLAVE_ID_DEFAULT                       {1, 2, }
#endif

/* MQTT */
#define MQTT_DATA_MAX_LENGTH                          1024
#define MQTT_TOPIC_MAX_LENGTH                         128
#define MQTT_MAX_SUBCRIBE_TOPIC                       8
#define MQTT_CLIENT_ID_LENGTH                         32
#define MQTT_MESSAGE_QUEUE_SIZE                       4
#define MQTT_QUEUE_MAX_DELAY_MS                       200

#define MQTT_BROKER_URI                              "mqtts://broker.emqx.io:8883"
#define MQTT_USERNAME                                "admin"
#define MQTT_PASSWORD                                "123456"

/* JSON */
#define JSON_METER_TYPE_KEY                           "meter"
#define JSON_SLAVE_ID_KEY                             "slave"
#define JSON_REG_KEY                                  "regs"
#define JSON_ADDRESS_KEY                              "address"
#define JSON_NAME_KEY                                 "key"
#define JSON_VALUE_KEY                                "value"

/* TASK */
#define MODBUS_TASK_NAME                              "modbus"
#define MODBUS_TASK_SIZE                              4096
#define MODBUS_TASK_PRIORITY                          3

#define MQTT_TASK_NAME                                "MQTT"
#define MQTT_TASK_SIZE                                4096
#define MQTT_TASK_PRIORITY                            4

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