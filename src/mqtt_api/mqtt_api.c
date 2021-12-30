/*
 *  mqtt_api.c
 *
 *  Created on: Sep 18, 2021
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <mqtt_client.h>
#include "config.h"
#include "wifi_lib/wifi_lib.h"
#include "mqtt_api.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

/*!
 * @brief  MQTT topic with callback function
 */
typedef struct {
    char topic[MQTT_TOPIC_MAX_LENGTH];
    mqtt_handle_t handler;
} topic_map_t;

/*!
 * @brief  MQTT message struct when using queue
 */
typedef struct {
    char message[MQTT_DATA_MAX_LENGTH];
    char topic[MQTT_TOPIC_MAX_LENGTH];
} mqtt_message_t;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static const char* TAG = "MQTT";

static esp_mqtt_client_handle_t mqtt_client;
static bool mqtt_broker_connected = false;
static char gateway_id[MQTT_CLIENT_ID_LENGTH] = "ESP-12345678";
static topic_map_t topic_list[MQTT_MAX_SUBCRIBE_TOPIC];
static uint8_t numb_topic = 0;
static QueueHandle_t mqtt_message_queue;

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/

extern const uint8_t ca_cert_pem_start[] asm("_binary_ca_cert_crt_start");
extern const uint8_t ca_cert_pem_end[] asm("_binary_ca_cert_crt_end");

/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

static esp_err_t mqtt_client_event_handler(esp_mqtt_event_handle_t event);
static void mqtt_handle_message_task(void* arg);

/******************************************************************************/

/*!
 * @brief  MQTT event handler
 * @param  None
 * @retval None
 */
static esp_err_t mqtt_client_event_handler(esp_mqtt_event_handle_t event)
{
    mqtt_client = event->client;

    switch(event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        mqtt_broker_connected = true;
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        for(uint8_t i = 0; i < numb_topic; i++)
        {
            esp_mqtt_client_subscribe(mqtt_client, topic_list[i].topic, 0);
            ESP_LOGI(TAG, "MQTT subcribe topic %s", topic_list[i].topic);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        mqtt_broker_connected = false;
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        if((event->data_len > 0) && (event->topic_len > 0))
        {
            ESP_LOGI(TAG, "-------- TOPIC = %.*s", event->topic_len, event->topic);
            mqtt_message_t message;
            sprintf(message.message, "%.*s", event->data_len, event->data);
            sprintf(message.topic, "%.*s", event->topic_len, event->topic);
            if (xQueueSend(mqtt_message_queue, &message, MQTT_QUEUE_MAX_DELAY_MS) != pdTRUE)
            {
                ESP_LOGW(TAG, "Send to mqtt message queue fail");
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;

    default:
        break;
    }

    return ESP_OK;
}

/*!
 * @brief  MQTT handle message receive task
 * @param  None
 * @retval None
 */
static void mqtt_handle_message_task(void* arg)
{
    mqtt_message_t message;
    uint32_t message_count = 0;
    bool is_exec;

    /* Task will suppend at here util network up */
    wifi_lib_wait_network_up();

    /* Start MQTT client */
    ESP_LOGI(TAG, "Start MQTT client");
    esp_mqtt_client_start(mqtt_client);

    while(1)
    {
        /* Wait message receive */
        if(xQueueReceive(mqtt_message_queue, &message, portMAX_DELAY) == pdTRUE)
        {
            is_exec = false;
            message_count++;
            ESP_LOGI(TAG, "-------- %u_DATA %s", message_count, message.message);
            for(uint8_t i = 0; i < numb_topic; i++) {
                if(strncmp(message.topic, topic_list[i].topic, strlen(topic_list[i].topic)) == 0) 
                {
                    if(topic_list[i].handler != NULL) 
                    {
                        topic_list[i].handler(message.message, strlen(message.message));
                        is_exec = true;
                    }
                }
            }
            if(!is_exec) 
            {
                ESP_LOGW(TAG, "message not handle!!!");
            }
            vTaskDelay(100 / portTICK_RATE_MS);
        }
    }
    vTaskDelete(NULL);
}

/******************************************************************************/

/*!
 * @brief  Client to send a publish message to the broker
 */
bool mqtt_api_publish(const char* topic, const char* data, uint32_t len)
{
    if(mqtt_broker_connected)
    {
        int32_t msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, len, 0, 0);
        ESP_LOGI(TAG, "Sent publish successful, msg_id = %d", msg_id);
        return true;
    }

    return false;
}

/*!
 * @brief  Register callback handle data reveived from topic
 */
bool mqtt_register_callback(char* topic, mqtt_handle_t func) {
    if (numb_topic >= MQTT_MAX_SUBCRIBE_TOPIC) {
        return false;
    }

    if (topic == NULL || func == NULL) {
        return false;
    }

    /* Register callback */
    topic_list[numb_topic].handler = func;
    snprintf(topic_list[numb_topic].topic, MQTT_TOPIC_MAX_LENGTH, "%s", topic);
    numb_topic++;

    ESP_LOGI(TAG, "Add topic [%s] to subcribe list", topic);
    return true;
}

/*!
 * @brief  MQTT client (transport over TCP) initialization and start
 */
void mqtt_api_init(void) {    
    /* Creat message queue */
    mqtt_message_queue = xQueueCreate(MQTT_MESSAGE_QUEUE_SIZE, sizeof(mqtt_message_t));
    if(mqtt_message_queue == NULL)
    {
        ESP_LOGE(TAG, "Create message queue fail");
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_BROKER_URI,
        .username = MQTT_USERNAME,
        .password = MQTT_PASSWORD,
        .cert_pem = (const char *) ca_cert_pem_start,
        .skip_cert_common_name_check = true,
        .disable_clean_session = true,
        .client_id = gateway_id,
        .event_handle = mqtt_client_event_handler,
    };

    /* Start mqtt client */
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    /* Creat mqtt handle message task */
    BaseType_t result = xTaskCreate(mqtt_handle_message_task, MQTT_TASK_NAME, MQTT_TASK_SIZE,
                                    NULL, MQTT_TASK_PRIORITY, NULL);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Create mqtt task fail %d", result);
    }
}