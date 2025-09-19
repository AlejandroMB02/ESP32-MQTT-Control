#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "dht.h"

#define SENSOR_TYPE DHT_TYPE_DHT11
#define DHT_GPIO_PIN GPIO_NUM_22

static const char *TAG = "MQTT_WEATHER";
#define  EXAMPLE_ESP_WIFI_SSID "<EXAMPLE_ESP_WIFI_SSID>"
#define  EXAMPLE_ESP_WIFI_PASS "<EXAMPLE_ESP_WIFI_PASS>"

#define MQTT_BROKER_URI "mqtt://<BROKER_IP>:1883"
#define MQTT_USER "<EXAMPLE_USER>"
#define MQTT_PASS "<EXAMPLE_PASS>"

static esp_mqtt_client_handle_t client = NULL;
static uint32_t MQTT_CONNECTED = 0;

/* ------------ Funciones ------------ */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data);

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);

void wifi_init(void);

void publisher_task(void *params);

/*Obtención de parametros TEMPERATUA / HUMEDAD*/
void dht_test(void *pvParameters)
{
    float temperature, humidity;

    while (1)
    {
        if (dht_read_float_data(SENSOR_TYPE, DHT_GPIO_PIN, &humidity, &temperature) == ESP_OK)
            printf("Humidity: %.1f%% Temp: %.1fC\n", humidity, temperature);
        else
            printf("Could not read data from sensor\n");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    gpio_reset_pin(GPIO_NUM_23);
    gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_23, 0);

    // xTaskCreate(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();
    xTaskCreate(publisher_task, "publisher_task", 1024 * 5, NULL, 5, NULL);
}



/* ------------ Manejador de eventos WiFi/IP ------------ */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Conectando al WiFi...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "WiFi conectado");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi desconectado, reintentando...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));
        // Arrancamos el cliente MQTT una vez tenemos IP
        if (client == NULL) {
            ESP_LOGI(TAG, "Iniciando cliente MQTT...");
            esp_mqtt_client_config_t mqtt_cfg = {
                .broker.address.uri = MQTT_BROKER_URI,
                .credentials.username = MQTT_USER,
                .credentials.authentication.password = MQTT_PASS,
            };
            client = esp_mqtt_client_init(&mqtt_cfg);
            esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
            esp_mqtt_client_start(client);
        }
    }
}

/* ------------ Inicialización WiFi ------------ */
void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta terminado.");
}

/* ------------ Manejador de eventos MQTT ------------ */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        MQTT_CONNECTED = 1;
        msg_id = esp_mqtt_client_subscribe(client, "/topic/test1", 0);
        ESP_LOGI(TAG, "Suscrito a /topic/test1, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, "/topic/test2", 1);
        ESP_LOGI(TAG, "Suscrito a /topic/test2, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/led", 0);
        ESP_LOGI(TAG, "Suscrito a /topic/led, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        MQTT_CONNECTED = 0;
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
        printf("TOPIC=%.*s DATA=%.*s\n", event->topic_len, event->topic,
                                    event->data_len, event->data);

        if (strncmp(event->topic, "/topic/led", event->topic_len) == 0 &&
            event->topic_len == strlen("/topic/led")) {

            if (strncmp(event->data, "ON", event->data_len) == 0) {
                gpio_set_level(GPIO_NUM_23, 1);
                ESP_LOGI(TAG, "LED ENCENDIDO");
            } else if (strncmp(event->data, "OFF", event->data_len) == 0) {
                gpio_set_level(GPIO_NUM_23, 0);
                ESP_LOGI(TAG, "LED APAGADO");
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other MQTT event id:%d", event->event_id);
        break;
    }
}

/* ------------ Tarea publicadora ------------ */
void publisher_task(void *params)
{
    float temperature, humidity;
    char msg[32];
    while (true) {
        if (MQTT_CONNECTED) {
            esp_mqtt_client_publish(client, "/topic/test3", "Hello World", 0, 0, 0);

            if (dht_read_float_data(SENSOR_TYPE, DHT_GPIO_PIN, &humidity, &temperature) == ESP_OK){
                printf("Humidity: %.1f%% Temp: %.1fC\n", humidity, temperature);
                snprintf(msg, sizeof(msg), "%.2f", temperature);
                esp_mqtt_client_publish(client, "/topic/temperature", msg, 0, 0, 0);

                snprintf(msg, sizeof(msg), "%.2f", humidity);
                esp_mqtt_client_publish(client, "/topic/humidity", msg, 0, 0, 0);
            }
            else
            printf("Could not read data from sensor\n");
        }
        vTaskDelay(15000 / portTICK_PERIOD_MS);
    }
}
