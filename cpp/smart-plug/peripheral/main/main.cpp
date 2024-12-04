#include <cmath>
#include "WifiService.h"
#include "BleAdvertiser.h"
#include "Timer.h"
#include "AcDimmer.h"
#include "MqttClient.h"
#include "AdtService.h"
#include "PlugConfig.h"

#include <string.h>
#include <inttypes.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

// BLE constants
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

// Define the ADT service and characteristic UUIDs
#define ADT_SERVICE_UUID                                "cc3aab60-0001-0000-81e0-c88f19fb28cb"
#define ADT_SERVICE_MTU_CHARACTERISTIC_UUID             "cc3aab60-0001-0001-81e0-c88f19fb28cb"
#define ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID    "cc3aab60-0001-0002-81e0-c88f19fb28cb"
#define ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID         "cc3aab60-0001-0003-81e0-c88f19fb28cb"

static const char *TAG = "SMART_PLUG";

void attemptConnectToWifi(shared_ptr<WifiConfig> wifiConfig);
void onWifiConnected(shared_ptr<int> connectionAttempt);
void onWifiDisconnected(shared_ptr<WifiConfig> wifiConfig, shared_ptr<int> connectionAttempt);

extern "C" void app_main(void)
{
    // Read the current configuration from NVS
    auto [error, config] = PlugConfig::readPlugConfig("namespace", "key");
    if (error != ESP_OK) {
        // TODO: We should go to some error state
        ESP_LOGE(TAG, "failed to read plug configuration, error code: %s", esp_err_to_name(error));
        return;
    }

    // Create a default plug configuration if one doesn't already exist
    if (config == nullptr) {
        ESP_LOGI(TAG, "no plug configuration found, creating a default one");
        config = make_unique<PlugConfig>(
            PlugConfig {
                // Config for BLE
                make_shared<BleConfig>(
                    "Smart Plug"
                ),
                // Config for non-dimmable LED
                make_shared<AcDimmerConfig>(
                    32,
                    25,
                    1000,
                    5000,
                    1200,
                    0
                ),
                // TODO: Remove wifi config in prod
                make_shared<WifiConfig>(
                    "denhac",
                    "denhac rule"
                ),
                // nullptr,
                // Config for MQTT
                make_shared<MqttConfig>(
                    "mqtt://10.11.2.96:1883"
                )
            }
        );
    }

    // // Serialize and deserialize the plug configuration (for testing and debugging purposes)
    // ESP_LOGI(TAG, "plug configuration: %s", cJSON_Print(config->serialize().get()));
    // string serialized = cJSON_Print(config->serialize().get());
    // PlugConfig deserialized = PlugConfig::deserialize(serialized);
    // ESP_LOGI(TAG, "deserialized plug configuration: %s", cJSON_Print(deserialized.serialize().get()));

    // Initiate the ac dimmer
    ESP_LOGI(TAG, "initializing ac dimmer");
    AcDimmer acDimmer(
        config.get()->acDimmerConfig->zcPin,
        config.get()->acDimmerConfig->psmPin,
        config.get()->acDimmerConfig->debounceUs,
        config.get()->acDimmerConfig->offsetLeading,
        config.get()->acDimmerConfig->offsetFalling,
        config.get()->acDimmerConfig->brightness
    );

    // Create the ADT service
    ESP_LOGI(TAG, "creating adt service");
    AdtService adtService(
        ADT_SERVICE_UUID,
        ADT_SERVICE_MTU_CHARACTERISTIC_UUID,
        ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID,
        ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID,
        [&acDimmer](vector<byte> message) {
            // TODO: Implement the message handler
            ESP_LOGI(TAG, "received message of length %d", message.size());
        }
    );

    // Initialize the BLE advertiser
    ESP_LOGI(TAG, "initializing ble advertiser");
    BleAdvertiser::init(
        config.get()->bleConfig->deviceName,
        BLE_GAP_APPEARANCE_GENERIC_TAG,
        BLE_GAP_LE_ROLE_PERIPHERAL,
        {
            adtService.getBleService()
        },
        [&adtService](shared_ptr<BleDevice> device) {
            // TODO: Implement the device connection handler
            ESP_LOGI(TAG, "device connected");
        }
    );

    // Initialize the wifi service
    ESP_LOGI(TAG, "initializing wifi service");
    WifiService::init();
    shared_ptr<int> connectionAttempt(0);
    shared_ptr<WifiConfig> wifiConfig = config.get()->wifiConfig;
    WifiService::onConnect = [connectionAttempt]() {
        onWifiConnected(connectionAttempt);
    };
    WifiService::onDisconnect = [wifiConfig, connectionAttempt]() {
        onWifiDisconnected(wifiConfig, connectionAttempt);
    };

    // Create an MQTT client
    ESP_LOGI(TAG, "creating mqtt client");
    Mqtt::MqttClient mqttClient("mqtt://10.11.2.96:1883");

    // Try and connect to wifi if a wifi configuration is present
    if(config.get()->wifiConfig != nullptr) {
        attemptConnectToWifi(config.get()->wifiConfig);
    } else {
        ESP_LOGI(TAG, "no wifi configuration found, skipping wifi connection");
    }
}

void attemptConnectToWifi(shared_ptr<WifiConfig> wifiConfig) {
    ESP_LOGW(TAG, "connecting to wifi");
    WifiService::startConnect({
        wifiConfig->ssid,
        wifiConfig->password,
    });
}

void onWifiConnected(shared_ptr<int> connectionAttempt) {
    ESP_LOGI(TAG, "wifi connected");
    *connectionAttempt = 0;
}

void onWifiDisconnected(shared_ptr<WifiConfig> wifiConfig, shared_ptr<int> connectionAttempt) {
    // Calculate the amount of time to wait before attempting to reconnect
    ESP_LOGW(TAG, "wifi disconnected on connection attempt %d", *connectionAttempt);
    int waitSeconds = pow(2, *connectionAttempt);

    // Wait for the specified duration
    ESP_LOGW(TAG, "wifi disconnected, waiting %d seconds before reconnecting", waitSeconds);
    vTaskDelay(pdMS_TO_TICKS(waitSeconds * 1000));

    // Attempt to reconnect to wifi
    attemptConnectToWifi(wifiConfig);

    // Increase the connection attempt count if the waitSeconds is less than 1800 (30 minutes)
    if (waitSeconds > 1800) {
        return;
    }

    (*connectionAttempt)++;
    ESP_LOGW(TAG, "increasing connection attempt to %d", *connectionAttempt);
}