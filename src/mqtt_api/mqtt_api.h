/*
 *  mqtt_api.h
 *
 *  Created on: Sep 18, 2021
 */

#ifndef __MQTT_API_H
#define __MQTT_API_H

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <stdint.h>

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define MQTT_AUTO_LENGTH                              0

typedef void (*mqtt_handle_t)(char*, uint32_t);

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
 * @brief  start MQTT client
 */
void mqtt_api_init(void);

/*!
 * @brief  publish message to the broker
 * @param  data: payload string (set to NULL, sending empty payload message)
 * @param  len:  data length, if set to 0, length is calculated from payload string
 * @retval true if success
 */
bool mqtt_api_publish(const char* topic, const char* data, uint32_t len);


/*!
 * @brief  register calback function for topic
 * @param  topic : topic name
 * @param  func  : callback function
 * @retval true when resgister success, otherwise return false
 */
bool mqtt_register_callback(char* topic, mqtt_handle_t func);

/******************************************************************************/

#endif // __MQTT_API_H
