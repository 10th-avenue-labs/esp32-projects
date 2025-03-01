#include "WifiService.h"
#include "MqttClient.h"

#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <mqtt_client.h>
#include <esp_event.h>

static char *TAG = "main";
static char *SSID = "IP-in-the-hot-tub";
static char *PASSWORD = "everytime";

int maxReconnectAttempts = 5;
int reconnectAttempts = 0;
void wifiInit()
{
    // Initialize the wifi service
    ESP_LOGI(TAG, "initializing wifi service");
    WifiService::WifiService::init();

    // Set the onDisconnect delegate
    WifiService::WifiService::onDisconnect = []()
    {
        ESP_LOGI(TAG, "wifi disconnected");

        // Attempt to reconnect
        if (reconnectAttempts < maxReconnectAttempts)
        {
            ESP_LOGI(TAG, "attempting to reconnect, attempt %d of %d", reconnectAttempts + 1, maxReconnectAttempts);
            reconnectAttempts++;
            WifiService::WifiService::startConnect({SSID,
                                                    PASSWORD});
        }
        else
        {
            ESP_LOGE(TAG, "failed to reconnect after %d attempts", maxReconnectAttempts);
        }
    };

    // Set the onConnect delegate
    WifiService::WifiService::onConnect = []()
    {
        ESP_LOGI(TAG, "wifi connected");
        reconnectAttempts = 0;
    };

    // Connect to the access point
    ESP_LOGI(TAG, "connecting to access point");
    WifiService::WifiService::startConnect({SSID,
                                            PASSWORD});
}

void onConnect(Mqtt::MqttClient *client)
{
    ESP_LOGI(TAG, "connected to broker");
}

int retries = 3;
void onDisconnect(Mqtt::MqttClient *client)
{
    ESP_LOGI(TAG, "disconnected from broker with retries: %d", retries);

    if (retries == 0)
    {
        return;
    }

    if (retries == 1)
    {
        client->setBrokerUri("mqtt://test.mosquitto.org:1883");
    }

    client->reconnect();

    retries--;
}

extern "C" int app_main(void)
{
    // Initialize the wifi service
    wifiInit();

    // Wait for the wifi to connect
    ESP_LOGI(TAG, "waiting for wifi to connect");
    WifiService::WifiService::waitConnectionState({WifiService::ConnectionState::CONNECTED});

    // Create the mqtt client
    Mqtt::MqttClient client("mqtt://nope.doesntexist");
    // Mqtt::MqttClient client("mqtt://test.mosquitto.org:1883");

    // Set the onConnected delegate
    client.onConnected = onConnect;

    // Set the onDisconnected delegate
    client.onDisconnected = onDisconnect;

    // Start the mqtt client
    client.connect();

    // Wait for the mqtt client to connect
    ESP_LOGI(TAG, "waiting for mqtt client to connect");
    client.waitConnectionState({Mqtt::ConnectionState::CONNECTED});

    // Subscribe to a topic
    int messageId = client.subscribe("/topic/qos0", [](Mqtt::MqttClient *client, int messageId, std::vector<std::byte> data) -> void
                                     {
        ESP_LOGI(TAG, "message received");

        // Convert the data to a string
        std::string message(reinterpret_cast<const char*>(data.data()), data.size());

        // Print out each byte
        for (std::byte b : data)
        {
            ESP_LOGI(TAG, "byte: %d", static_cast<int>(b));
        }

        const char* cStr = message.c_str();
        // Print out each character until the null terminator
        for (int i = 0; cStr[i] != '\0'; i++)
        {
            ESP_LOGI(TAG, "char: %c", cStr[i]);
        }

        // Log the message
        ESP_LOGI(TAG, "message: %d, %s", data.size(), message.c_str()); });
    ESP_LOGI(TAG, "subscribed to topic, message id: %d", messageId);

    // Publish a byte vector message to a topic
    std::string message = "hello";
    std::vector<std::byte> data(message.size());
    std::transform(message.begin(), message.end(), data.begin(),
                   [](char c)
                   { return static_cast<std::byte>(c); });
    int msg1Id = client.publish("/topic/qos0", data);
    ESP_LOGI(TAG, "published message, message id: %d", msg1Id);

    // Delay for a second (this seems to be somewhat necessary for such quick publishes back-to-back)
    ESP_LOGI(TAG, "delaying for a few seconds");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Publish a string message to a topic
    int msg2Id = client.publish("/topic/qos0", "world");
    ESP_LOGI(TAG, "published message, message id: %d", msg2Id);

    // Delay for a second
    ESP_LOGI(TAG, "delaying for a few seconds again");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Unsubscribe from a topic
    messageId = client.unsubscribe("/topic/qos0");
    ESP_LOGI(TAG, "unsubscribed from topic, message id: %d", messageId);

    // Disconnect the mqtt client
    ESP_LOGI(TAG, "disconnecting from broker");
    client.disconnect();

    // Wait indefinitely
    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    return 0;
}