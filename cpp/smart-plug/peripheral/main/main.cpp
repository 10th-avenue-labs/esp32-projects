#include "WifiService.h"
#include "BleAdvertiser.h"
#include "Timer.h"
#include "AcDimmer.h"
#include "MqttClient.h"
#include "include/smart_plug.h"

extern "C" {
    #include <string.h>
    #include <inttypes.h>
    #include "esp_wifi.h"
    #include "esp_event.h"
    #include "esp_log.h"
}

static const char *TAG = "smart plug";

// BLE constants
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

// Wifi service
static ble_uuid16_t wifiServiceUuid = BLE_UUID16_INIT(0x3245);
static ble_uuid128_t wifiSsidUuid =
    BLE_UUID128_INIT(0x32, 0x45, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);
static ble_uuid128_t wifiPasswordUuid =
    BLE_UUID128_INIT(0x32, 0x45, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x01);
static ble_uuid128_t wifiConnectionStateUuid =
    BLE_UUID128_INIT(0x32, 0x45, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x02);

// Dimmer service
static ble_uuid16_t dimmerServiceUuid = BLE_UUID16_INIT(0x3246);
static ble_uuid128_t dimmerBrightnessUuid =
    BLE_UUID128_INIT(0x32, 0x46, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

class WifiConfig {
    public:
        std::string ssid;
        std::string password;
};

void bleInit(AcDimmer* acDimmer, WifiConfig* wifiConfig) {
    // Create the services
    ESP_LOGI(TAG, "creating BLE Service for Wifi");
    std::vector<BleService> services = {
        // Create the wifi service
        BleService(
            &wifiServiceUuid,
            {
                // Create the SSID characteristic
                BleCharacteristic(
                    &wifiSsidUuid,
                    [wifiConfig](std::vector<std::byte> data) -> int {
                        // Convert the data to a string
                        std::string ssid(reinterpret_cast<const char*>(data.data()), data.size());

                        // Set the SSID
                        ESP_LOGI(TAG, "setting SSID to %s", ssid.c_str());
                        wifiConfig->ssid = ssid;

                        // Attempt to connect to the wifi network
                        WifiService::startConnect(
                            {
                                wifiConfig->ssid,
                                wifiConfig->password
                            }
                        );

                        // Return success
                        return 0;
                    },
                    [wifiConfig]() -> std::vector<std::byte> {
                        // Get the SSID
                        std::string ssid = wifiConfig->ssid;

                        // Log the read request
                        ESP_LOGI(TAG, "reading SSID: %s", ssid.c_str());

                        // Convert the SSID to a vector of bytes
                        return std::vector<std::byte>(
                            reinterpret_cast<const std::byte*>(ssid.data()),
                            reinterpret_cast<const std::byte*>(ssid.data()) + ssid.size()
                        );
                    },
                    true, // Characteristic can be read
                    true, // Characteristic can be written
                    false // Characteristic does not acknowledge writes. We do this because the Dotnet BLE library does not support write acknowledgment
                ),
                // Create the password characteristic
                BleCharacteristic(
                    &wifiPasswordUuid,
                    [wifiConfig](std::vector<std::byte> data) -> int {
                        // Convert the data to a string
                        std::string password(reinterpret_cast<const char*>(data.data()), data.size());

                        // Set the password
                        ESP_LOGI(TAG, "setting password to %s", password.c_str());
                        wifiConfig->password = password;

                        // Attempt to connect to the wifi network
                        WifiService::startConnect(
                            {
                                wifiConfig->ssid,
                                wifiConfig->password
                            }
                        );

                        // Return success
                        return 0;
                    },
                    nullptr,
                    false, // Characteristic can not be read
                    true, // Characteristic can be written
                    false // Characteristic does not acknowledge writes. We do this because the Dotnet BLE library does not support write acknowledgment
                ),
                // Create a characteristic for the connection state
                BleCharacteristic(
                    &wifiConnectionStateUuid,
                    nullptr,
                    []() -> std::vector<std::byte> {
                        // Get the connection state
                        ConnectionState connectionState = WifiService::getConnectionState();

                        // Log the read request
                        ESP_LOGI(TAG, "reading connection state: %d", connectionState);

                        // Convert the connection state to a vector of bytes
                        return { (std::byte) connectionState };
                    },
                    true, // Characteristic can be read
                    false, // Characteristic can not be written
                    false // Characteristic does not acknowledge writes. We do this because the Dotnet BLE library does not support write acknowledgment
                )
            }
        ),
        // Create the dimmer service
        BleService(
            &dimmerServiceUuid,
            {
                // Create the brightness characteristic
                BleCharacteristic(
                    &dimmerBrightnessUuid,
                    [acDimmer](std::vector<std::byte> data) -> int {
                        // Convert the data to a uint8_t
                        uint8_t brightness = (uint8_t)data[0];

                        // Set the brightness
                        ESP_LOGI(TAG, "setting brightness to %d", brightness);
                        acDimmer->setBrightness(brightness);

                        // Return success
                        return 0;
                    },
                    [acDimmer]() -> std::vector<std::byte> {
                        // Get the brightness
                        uint8_t brightness = acDimmer->getBrightness();

                        // Log the read request
                        ESP_LOGI(TAG, "reading brightness: %d", brightness);

                        // Get the brightness and convert it to a vector of bytes
                        return { (std::byte) brightness };
                    },
                    true, // Characteristic can be read
                    true, // Characteristic can be written
                    false // Characteristic does not acknowledge writes. We do this because the Dotnet BLE library does not support write acknowledgment
                )
            }
        )
    };

    // Initialize the Ble Advertiser
    ESP_LOGI(TAG, "initializing BLE Advertiser");
    BleAdvertiser::init("Smart Plug", BLE_GAP_APPEARANCE_GENERIC_TAG, BLE_GAP_LE_ROLE_PERIPHERAL, services);
}

static uint8_t maxReconnectAttempts = 5;
static uint8_t reconnectAttempts = 0;
void wifiInit(WifiConfig* wifiConfig, Mqtt::MqttClient* mqttClient) {
    WifiService::onDisconnect = [wifiConfig]() {
        ESP_LOGI(TAG, "wifi disconnected");

        // Attempt to reconnect
        if (reconnectAttempts < maxReconnectAttempts) {
            ESP_LOGI(TAG, "attempting to reconnect");
            reconnectAttempts++;
            WifiService::startConnect(
                {
                    wifiConfig->ssid,
                    wifiConfig->password
                }
            );
        } else {
            ESP_LOGI(TAG, "max reconnect attempts reached");
        }
    };
    WifiService::onConnect = [mqttClient]() {
        ESP_LOGI(TAG, "Wifi connected");
        reconnectAttempts = 0;

        // Attempt to connect the mqtt client
        mqttClient->connect();
    };
    WifiService::init();
    WifiService::startConnect(
        {
            wifiConfig->ssid,
            wifiConfig->password
        }
    );
}

static void bleAdvertiserTask(void* args) {
    // Advertise the Ble Device
    ESP_LOGI(TAG, "advertising BLE Device");
    BleAdvertiser::advertise();

    vTaskDelete(NULL);
}

void onConnected(Mqtt::MqttClient* client, AcDimmer* acDimmer) {
    ESP_LOGI(TAG, "connected to mqtt broker");

    // Subscribe to the smart plug brightness topic
    client->subscribe(
        "smart-plug/brightness",
        [acDimmer](Mqtt::MqttClient* client, int messageId, std::vector<std::byte> data) {
            // Convert the data to a uint8_t
            uint8_t brightness = (uint8_t)data[0];

            // Set the brightness
            ESP_LOGI(TAG, "setting brightness to %d", brightness);
            acDimmer->setBrightness(brightness);
        }
    );
}

int maxBrokerReconnectAttempts = 5;
int brokerReconnectAttempts = 0;
void onDisconnected(Mqtt::MqttClient* client) {
    ESP_LOGI(TAG, "disconnected from mqtt broker");

    // Attempt to reconnect
    if (brokerReconnectAttempts < maxBrokerReconnectAttempts) {
        ESP_LOGI(TAG, "attempting to reconnect to mqtt broker");
        brokerReconnectAttempts++;
        client->connect();
    } else {
        ESP_LOGI(TAG, "max reconnect attempts reached");
    }
}

extern "C" void app_main(void)
{
    // Initiate the ac dimmer
    // Config for non-dimmable LED
    AcDimmer acDimmer(32, 25, 1000, 5000, 1200);

    // Create the configuration for the wifi service
    WifiConfig wifiConfig;

    // TODO: Implement NVS
    // Attempt to read the wifi configuration from NVS
    // Attempt to read the smart plug brightness from NVS

    // Create an MQTT client
    Mqtt::MqttClient mqttClient("mqtt://10.11.2.96:1883");

    // Set the onConnected delegate
    mqttClient.onConnected = [&acDimmer](Mqtt::MqttClient* client) {
        onConnected(client, &acDimmer);
    };

    // Set the onDisconnected delegate
    mqttClient.onDisconnected = onDisconnected;

    // Initiate the Wifi Service
    wifiInit(&wifiConfig, &mqttClient);

    // Initiate the Ble Advertiser
    bleInit(&acDimmer, &wifiConfig);

    // Create a task for the BLE advertiser
    xTaskCreate(bleAdvertiserTask, "ble_advertiser", 4096, NULL, 5, NULL);

    uint8_t brightness = 255;
    while(1) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // acDimmer.setBrightness(brightness);
        // brightness--;
    }
}
