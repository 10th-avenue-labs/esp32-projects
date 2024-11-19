#include "WifiService.h"

extern "C"
{
#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <mqtt_client.h>
#include <esp_event.h>
}

static char *TAG = "MQTT_CLIENT";
static char *SSID = "IP-in-the-hot-tub";
static char *PASSWORD = "everytime";

int maxReconnectAttempts = 5;
int reconnectAttempts = 0;
void wifiInit()
{
    // Initialize the wifi service
    ESP_LOGI(TAG, "initializing wifi service");
    WifiService::init();

    // Set the onDisconnect delegate
    WifiService::onDisconnect = []()
    {
        ESP_LOGI(TAG, "wifi disconnected");

        // Attempt to reconnect
        if (reconnectAttempts < maxReconnectAttempts)
        {
            ESP_LOGI(TAG, "attempting to reconnect, attempt %d of %d", reconnectAttempts + 1, maxReconnectAttempts);
            reconnectAttempts++;
            WifiService::startConnect({SSID,
                                       PASSWORD});
        }
        else
        {
            ESP_LOGE(TAG, "failed to reconnect after %d attempts", maxReconnectAttempts);
        }
    };

    // Set the onConnect delegate
    WifiService::onConnect = []()
    {
        ESP_LOGI(TAG, "wifi connected");
        reconnectAttempts = 0;
    };

    // Connect to the access point
    ESP_LOGI(TAG, "connecting to access point");
    WifiService::startConnect({SSID,
                               PASSWORD});
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data1234", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

extern "C" int app_main(void)
{
    // Initialize the wifi service
    wifiInit();

    // Wait for the wifi to connect
    ESP_LOGI(TAG, "waiting for wifi to connect");
    WifiService::waitConnectionState({ConnectionState::CONNECTED});

    // Create the mqtt configuration
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = "mqtt://test.mosquitto.org",
                // .uri = "mqtt://mqtt.eclipseprojects.io",
            }}};

    // Initialize the mqtt client
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

    // Register the event handler
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);

    // Start the mqtt client
    esp_mqtt_client_start(client);

    // Wait indefinitely
    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    return 0;
}