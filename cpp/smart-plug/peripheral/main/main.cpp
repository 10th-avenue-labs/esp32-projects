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
#include <PlugMessage.h>
#include <typeinfo>
#include <SetAcDimmerConfig.h>
#include <SetWifiConfig.h>
#include <SetMqttConfig.h>
#include <SetBleConfig.h>

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
void onWifiConnected(shared_ptr<Mqtt::MqttClient> client, shared_ptr<int> connectionAttempt);
void onWifiDisconnected(shared_ptr<WifiConfig> wifiConfig, shared_ptr<int> connectionAttempt);

void attemptConnectToMqtt(shared_ptr<Mqtt::MqttClient> client);
void onMqttConnected(Mqtt::MqttClient* client, shared_ptr<int> connectionAttempt);
void onMqttDisconnected(Mqtt::MqttClient* client, shared_ptr<int> connectionAttempt);

void createMqttClient(shared_ptr<MqttConfig> mqttConfig, shared_ptr<Mqtt::MqttClient>& mqttClient, shared_ptr<int> mqttConnectionAttempt);
void updateMqttClient(shared_ptr<MqttConfig> mqttConfig, shared_ptr<Mqtt::MqttClient> mqttClient, shared_ptr<int> mqttConnectionAttempt);

extern "C" void app_main(void)
{
    // Register the Plug message deserializers
    PlugMessage::registerDeserializer("SetAcDimmerConfig", SetAcDimmerConfig::deserialize);
    PlugMessage::registerDeserializer("SetWifiConfig", SetWifiConfig::deserialize);
    PlugMessage::registerDeserializer("SetMqttConfig", SetMqttConfig::deserialize);
    PlugMessage::registerDeserializer("SetBleConfig", SetBleConfig::deserialize);

    // Create a message
    string setAcDimmerMessage = "{\"type\":\"SetAcDimmerConfig\",\"data\":{\"brightness\":50}}";

    // Deserialize the message
    PlugMessage deserializedAcDimmerMessage = PlugMessage::deserialize(setAcDimmerMessage);

    // Cast the message to a SetAcDimmerConfig
    SetAcDimmerConfig* setAcDimmerConfig = dynamic_cast<SetAcDimmerConfig*>(deserializedAcDimmerMessage.message.get());

    // Check if the cast was successful
    if (setAcDimmerConfig != nullptr) {
        ESP_LOGI(TAG, "brightness: %d", setAcDimmerConfig->brightness);
    } else {
        ESP_LOGE(TAG, "failed to cast message to SetAcDimmerConfig");
    }

    // Create a message
    string setWifiMessage = "{\"type\":\"SetWifiConfig\",\"data\":{\"ssid\":\"denhac\",\"password\":\"denhac rules\"}}";

    // Deserialize the message
    PlugMessage deserializedWifiMessage = PlugMessage::deserialize(setWifiMessage);

    // Cast the message to a SetWifiConfig
    SetWifiConfig* setWifiConfig = dynamic_cast<SetWifiConfig*>(deserializedWifiMessage.message.get());

    // Check if the cast was successful 
    if (setWifiConfig != nullptr) {
        ESP_LOGI(TAG, "ssid: %s, password: %s", setWifiConfig->ssid.c_str(), setWifiConfig->password.c_str());
    } else {
        ESP_LOGE(TAG, "failed to cast message to SetWifiConfig");
    }

    // Create a message
    string setMqttMessage = "{\"type\":\"SetMqttConfig\",\"data\":{\"brokerAddress\":\"addy\"}}";

    // Deserialize the message
    PlugMessage deserializedMqttMessage = PlugMessage::deserialize(setMqttMessage);

    // Cast the message to a SetMqttConfig
    SetMqttConfig* setMqttConfig = dynamic_cast<SetMqttConfig*>(deserializedMqttMessage.message.get());

    // Check if the cast was successful
    if (setMqttConfig != nullptr) {
        ESP_LOGI(TAG, "brokerAddress: %s", setMqttConfig->brokerAddress.c_str());
    } else {
        ESP_LOGE(TAG, "failed to cast message to SetMqttConfig");
    }

    // Create a message
    string setBleMessage = "{\"type\":\"SetBleConfig\",\"data\":{\"deviceName\":\"name\"}}";

    // Deserialize the message
    PlugMessage deserializedBleMessage = PlugMessage::deserialize(setBleMessage);

    // Cast the message to a SetBleConfig
    SetBleConfig* setBleConfig = dynamic_cast<SetBleConfig*>(deserializedBleMessage.message.get());

    // Check if the cast was successful
    if (setBleConfig != nullptr) {
        ESP_LOGI(TAG, "deviceName: %s", setBleConfig->deviceName.c_str());
    } else {
        ESP_LOGE(TAG, "failed to cast message to SetBleConfig");
    }

    return;

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
                    "denhac rules"
                ),
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

    // Create an MQTT client
    shared_ptr<int> mqttConnectionAttempt = make_shared<int>(0);
    shared_ptr<Mqtt::MqttClient> mqttClient;
    
    // Check if the MQTT configuration is present
    if (config.get()->mqttConfig != nullptr) {
        createMqttClient(config.get()->mqttConfig, mqttClient, mqttConnectionAttempt);
    } else {
        ESP_LOGI(TAG, "no mqtt configuration found, skipping mqtt connection");
    }

    // Initialize the wifi service
    ESP_LOGI(TAG, "initializing wifi service");
    WifiService::init();
    shared_ptr<int> wifiConnectionAttempt = make_shared<int>(0);
    shared_ptr<WifiConfig> wifiConfig = config.get()->wifiConfig;
    WifiService::onConnect = [mqttClient, wifiConnectionAttempt]() {
        onWifiConnected(mqttClient, wifiConnectionAttempt);
    };
    WifiService::onDisconnect = [wifiConfig, wifiConnectionAttempt]() {
        onWifiDisconnected(wifiConfig, wifiConnectionAttempt);
    };

    // Try and connect to wifi if a wifi configuration is present
    if(config.get()->wifiConfig != nullptr) {
        attemptConnectToWifi(config.get()->wifiConfig);
    } else {
        ESP_LOGI(TAG, "no wifi configuration found, skipping wifi connection");
    }
}

////////////////////////////////////////////////////////////////////////////////
/// WiFi Connection Handlers
////////////////////////////////////////////////////////////////////////////////

void attemptConnectToWifi(shared_ptr<WifiConfig> wifiConfig) {
    ESP_LOGI(TAG, "connecting to wifi");
    WifiService::startConnect({
        wifiConfig->ssid,
        wifiConfig->password,
    });
}

void onWifiConnected(shared_ptr<Mqtt::MqttClient> client, shared_ptr<int> connectionAttempt) {
    ESP_LOGI(TAG, "wifi connected");

    // Reset the connection attempt counter
    *connectionAttempt = 0;

    // Attempt to connect to MQTT
    attemptConnectToMqtt(client);
}

void onWifiDisconnected(shared_ptr<WifiConfig> wifiConfig, shared_ptr<int> connectionAttempt) {
    // Calculate the amount of time to wait before attempting to reconnect
    int waitSeconds = pow(2, *connectionAttempt);

    // Wait for the specified duration
    ESP_LOGW(TAG, "wifi disconnected on connection attempt %d, waiting %d seconds before reconnecting", *connectionAttempt, waitSeconds);
    vTaskDelay(pdMS_TO_TICKS(waitSeconds * 1000));

    // Attempt to reconnect to wifi
    attemptConnectToWifi(wifiConfig);

    // If the wait seconds is already larger than 1800 (30 minutes), do not increase further.
    if (waitSeconds > 1800) {
        return;
    }

    (*connectionAttempt)++;
}

////////////////////////////////////////////////////////////////////////////////
/// Create or update the MqttClient
////////////////////////////////////////////////////////////////////////////////

void createMqttClient(shared_ptr<MqttConfig> mqttConfig, shared_ptr<Mqtt::MqttClient>& mqttClient, shared_ptr<int> mqttConnectionAttempt) {
    ESP_LOGI(TAG, "creating mqtt client");

    // Create the mqtt client
    mqttClient = make_shared<Mqtt::MqttClient>(
        mqttConfig->brokerAddress
    );
    mqttClient->onConnected = [mqttConnectionAttempt](Mqtt::MqttClient* client){onMqttConnected(client, mqttConnectionAttempt);};
    mqttClient->onDisconnected = [mqttConnectionAttempt](Mqtt::MqttClient* client){onMqttDisconnected(client, mqttConnectionAttempt);};
}

void updateMqttClient(shared_ptr<MqttConfig> mqttConfig, shared_ptr<Mqtt::MqttClient> mqttClient, shared_ptr<int> mqttConnectionAttempt) {
    // Check if an MQTT client is present
    if (mqttClient == nullptr) {
        createMqttClient(mqttConfig, mqttClient, mqttConnectionAttempt);
    } else {
        ESP_LOGI(TAG, "updating mqtt client");
        mqttClient->setBrokerUri(mqttConfig->brokerAddress);
    }

    // Attempt to connect to MQTT
    attemptConnectToMqtt(mqttClient);
}

////////////////////////////////////////////////////////////////////////////////
/// MQTT Connection Handlers
////////////////////////////////////////////////////////////////////////////////

void attemptConnectToMqtt(shared_ptr<Mqtt::MqttClient> client) {
    // Check if the MQTT client is present
    if (client == nullptr) {
        ESP_LOGW(TAG, "no mqtt client found, skipping mqtt connection");
        return;
    }

    ESP_LOGI(TAG, "connecting to mqtt");
    // NOTE: We must ensure that we're connected to wifi before trying to connect the MQTT client
    // Trying to connect when not connected to wifi will result in an unrecoverable error
    // We can do nothing to handle this error that the device will reboot
    client->connect();
}

void onMqttConnected(Mqtt::MqttClient* client, shared_ptr<int> connectionAttempt) {
    ESP_LOGI(TAG, "mqtt connected");
    *connectionAttempt = 0;
}

void onMqttDisconnected(Mqtt::MqttClient* client, shared_ptr<int> connectionAttempt) {
    // Check if wifi is connected
    if (WifiService::getConnectionState() != ConnectionState::CONNECTED) {
        ESP_LOGW(TAG, "wifi is not connected, skipping mqtt reconnection");
        *connectionAttempt = 0;
        return;
    }

    // Calculate the amount of time to wait before attempting to reconnect
    int waitSeconds = pow(2, *connectionAttempt);

    // Wait for the specified duration
    ESP_LOGW(TAG, "mqtt disconnected on connection attempt %d, waiting %d seconds before reconnecting", *connectionAttempt, waitSeconds);
    vTaskDelay(pdMS_TO_TICKS(waitSeconds * 1000));

    // Attempt to reconnect to mqtt
    client->connect();

    // If the wait seconds is already larger than 1800 (30 minutes), do not increase further.
    if (waitSeconds > 1800) {
        return;
    }

    (*connectionAttempt)++;
}

////////////////////////////////////////////////////////////////////////////////
/// Adt message handler
////////////////////////////////////////////////////////////////////////////////

void adtMessageHandler(vector<byte> message) {
    // Convert the data to a string
    string messageString(
        reinterpret_cast<const char*>(message.data()),
        reinterpret_cast<const char*>(message.data()) + message.size()
    );

    // Deserialize the message
    PlugMessage plugMessage = PlugMessage::deserialize(messageString);

}

void setAcDimmerConfigHandler(PlugMessage message) {
    // // Deserialize the message
    // SetAcDimmerConfig config = SetAcDimmerConfig::deserialize(message.message);

    // // Set the ac dimmer configuration
    // acDimmer.setConfig(config);
}