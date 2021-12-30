/*
 *  wifi_lib.c
 *
 *  Created on: Oct 22, 2021
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "config.h"
#include "wifi_lib.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define NETWORK_GOT_IP_EVENT                              0x00000001

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

static const char* TAG = "WIFI";
static int32_t retry_num = 0;                        /* Retry to connect */
esp_netif_t *wifi_sta_netif = NULL;                  /* Wifi station interface */
static EventGroupHandle_t wifi_status_events;        /* Network up (wifi is connected) status */

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifi_got_ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/******************************************************************************/

/*!
 * @brief  Event handler for Wifi events
 * @param  Event data
 * @retval None
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch(event_id)
    {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Wifi Started");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(TAG, "Wifi Stopped");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Wifi is connected");
            retry_num = 0;
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            retry_num++;
            ESP_LOGI(TAG, "Wifi is disconnected. Retry %d", retry_num);
            vTaskDelay(WIFI_TIME_RETRY_CONNECT_MS / portTICK_PERIOD_MS);
            esp_wifi_connect();
            break;
        default:
            break;
    }
}

/*!
 * @brief  Event handler for IP_EVENT_ETH_GOT_IP
 * @param  Event data
 * @retval None
 */
static void wifi_got_ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "*********************** Wifi Got IP Address");
    ESP_LOGI(TAG, "*********************** WIFIIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "*********************** WIFIMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "*********************** WIFIGW:" IPSTR, IP2STR(&ip_info->gw));

    xEventGroupSetBits(wifi_status_events, NETWORK_GOT_IP_EVENT);
}

/******************************************************************************/

/*!
 * @brief  Wait util network up
 */
void wifi_lib_wait_network_up(void)
{
    xEventGroupWaitBits(wifi_status_events, NETWORK_GOT_IP_EVENT, false, false, portMAX_DELAY);
}

/*!
 * @brief  Wifi initialization in station mode
 */
void wifi_lib_init_sta(void)
{
    /* Initialize TCP/IP network interface (should be called only once in application) */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Create default event loop that running in background */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize Wi-Fi including netif with default config */
    wifi_sta_netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_got_ip_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    /* Start wifi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Network status event */
    wifi_status_events = xEventGroupCreate();
    ESP_LOGI(TAG, "Wifi is initialized");
}