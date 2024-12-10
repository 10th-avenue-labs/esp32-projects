#include <cmath>
#include "WifiService.h"
#include "BleAdvertiser.h"
#include "Timer.h"
#include "AcDimmer.h"
#include "MqttClient.h"
#include "AdtService.h"
#include "PlugConfig.h"
#include "Result.h"
#include "ISerializable.h"
#include "MessageResponse.h"

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

// Define the namespace and key for the plug configuration
#define PLUG_CONFIG_NAMESPACE "storage"
#define PLUG_CONFIG_KEY "config"

// BLE constants
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

// Define the ADT service and characteristic UUIDs
#define ADT_SERVICE_UUID                                "cc3aab60-0001-0000-81e0-c88f19fb28cb"
#define ADT_SERVICE_MTU_CHARACTERISTIC_UUID             "cc3aab60-0001-0001-81e0-c88f19fb28cb"
#define ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID    "cc3aab60-0001-0002-81e0-c88f19fb28cb"
#define ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID         "cc3aab60-0001-0003-81e0-c88f19fb28cb"

static const char *TAG = "SMART_PLUG";

// Initialization helpers
Result<shared_ptr<PlugConfig>> getConfigOrDefault();

// Wifi connection handlers
void attemptConnectToWifi(shared_ptr<WifiConfig> wifiConfig);
void onWifiConnected(shared_ptr<Mqtt::MqttClient> client, shared_ptr<int> connectionAttempt);
void onWifiDisconnected(shared_ptr<WifiConfig> wifiConfig, shared_ptr<int> connectionAttempt);

// MQTT connection handlers
void attemptConnectToMqtt(shared_ptr<Mqtt::MqttClient> client);
void onMqttConnected(Mqtt::MqttClient* client, shared_ptr<int> connectionAttempt);
void onMqttDisconnected(Mqtt::MqttClient* client, shared_ptr<int> connectionAttempt);

// Create of update the MQTT client
void createMqttClient(shared_ptr<MqttConfig> mqttConfig, shared_ptr<Mqtt::MqttClient>& mqttClient, shared_ptr<int> mqttConnectionAttempt);
void updateMqttClient(shared_ptr<MqttConfig> mqttConfig, shared_ptr<Mqtt::MqttClient> mqttClient, shared_ptr<int> mqttConnectionAttempt);

// ADT message handler
void adtMessageHandler(uint16_t messageId, vector<byte> message, shared_ptr<BleDevice> device);

// Message handlers
Result<shared_ptr<ISerializable>> bleConfigUpdateHandler(unique_ptr<IPlugMessageData> message);
Result<shared_ptr<ISerializable>> setAcDimmerConfigHandler(unique_ptr<IPlugMessageData> message);
Result<shared_ptr<ISerializable>> setWifiConfigHandler(unique_ptr<IPlugMessageData> message);
Result<shared_ptr<ISerializable>> setMqttConfigHandler(unique_ptr<IPlugMessageData> message);

// Meta functions
esp_err_t reset();

// Shared resources
shared_ptr<PlugConfig> config;
unique_ptr<AdtService> adtService;
unique_ptr<AcDimmer> acDimmer;

extern "C" void app_main(void)
{
    // reset();

    // Register the Plug message deserializers
    PlugMessage::registerDeserializer("SetAcDimmerConfig", SetAcDimmerConfig::deserialize);
    PlugMessage::registerDeserializer("SetWifiConfig", SetWifiConfig::deserialize);
    PlugMessage::registerDeserializer("SetMqttConfig", SetMqttConfig::deserialize);
    PlugMessage::registerDeserializer("SetBleConfig", SetBleConfig::deserialize);

    // Get config or default
    Result<shared_ptr<PlugConfig>> plugConfigResult = getConfigOrDefault();
    if (!plugConfigResult.success) {
        ESP_LOGE(TAG, "failed to get plug config, error: %s", plugConfigResult.getError().c_str());
        return;
    }
    config = move(plugConfigResult.getValue());

    // Log the configuration
    ESP_LOGI(TAG, "bleConfig: %s", cJSON_Print(config->serialize().get()));

    // Initiate the ac dimmer
    ESP_LOGI(TAG, "initializing ac dimmer");
    acDimmer = make_unique<AcDimmer>(
        config.get()->acDimmerConfig->zcPin,
        config.get()->acDimmerConfig->psmPin,
        config.get()->acDimmerConfig->debounceUs,
        config.get()->acDimmerConfig->offsetLeading,
        config.get()->acDimmerConfig->offsetFalling,
        config.get()->acDimmerConfig->brightness
    );

    // Create the ADT service
    ESP_LOGI(TAG, "creating adt service");
    adtService = make_unique<AdtService>(
        ADT_SERVICE_UUID,
        ADT_SERVICE_MTU_CHARACTERISTIC_UUID,
        ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID,
        ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID,
        adtMessageHandler
    );

    // Initialize the BLE advertiser
    ESP_LOGI(TAG, "initializing ble advertiser");
    BleAdvertiser::init(
        config.get()->bleConfig->deviceName,
        BLE_GAP_APPEARANCE_GENERIC_TAG,
        BLE_GAP_LE_ROLE_PERIPHERAL,
        {
            adtService->getBleService()
        },
        nullptr // No device connected handler needed
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

    // Begin advertising
    ESP_LOGI(TAG, "beginning advertising");
    BleAdvertiser::advertise();
}

////////////////////////////////////////////////////////////////////////////////
/// Initialization helpers
////////////////////////////////////////////////////////////////////////////////

Result<shared_ptr<PlugConfig>> getConfigOrDefault() {
    // Read the plug configuration from NVS if it exists
    Result<shared_ptr<PlugConfig>> plugConfigResult = PlugConfig::readPlugConfig(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY);
    if (!plugConfigResult.isSuccess()) {
        return plugConfigResult;
    }

    // Check if a configuration was found
    if (plugConfigResult.getValue() != nullptr) {
        ESP_LOGI(TAG, "found plug configuration");
        return plugConfigResult;
    }

    // Create a default plug configuration if one doesn't already exist
    // TODO: Consider defining the default function somewhere else
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
            // Config for WiFi
            nullptr,
            // Config for MQTT
            make_shared<MqttConfig>(
                "mqtt://10.11.2.96:1883"
            )
        }
    );

    return Result<shared_ptr<PlugConfig>>::createSuccess(move(config));
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

void adtMessageHandler(uint16_t messageId, vector<byte> message, shared_ptr<BleDevice> device) {
    // Convert the data to a string
    string messageString(
        reinterpret_cast<const char*>(message.data()),
        reinterpret_cast<const char*>(message.data()) + message.size()
    );

    // Deserialize the message
    PlugMessage plugMessage = PlugMessage::deserialize(messageString);

    // Create a map of message handlers
    unordered_map<string, function<Result<shared_ptr<ISerializable>>(unique_ptr<IPlugMessageData>)>> messageHandlers = {
        {"SetBleConfig", bleConfigUpdateHandler},
        {"SetAcDimmerConfig", setAcDimmerConfigHandler},
        {"SetWifiConfig", setWifiConfigHandler},
        {"SetMqttConfig", setMqttConfigHandler}
    };

    // Get the message handler
    auto messageHandler = messageHandlers.find(plugMessage.type);

    // Check if the message handler was found
    if (messageHandler == messageHandlers.end()) {
        ESP_LOGE(TAG, "no message handler found for message type %s", plugMessage.type.c_str());
        return;
    }

    // Call the message handler
    Result<shared_ptr<ISerializable>> result = messageHandler->second(move(plugMessage.data));

    // Create a response message
    PlugMessage response("Response", make_unique<MessageResponse<shared_ptr<ISerializable>>>(messageId, result));

    // Serialize the response message
    string serializedResponse = response.serialize();

    // Convert the message to a vector of bytes
    vector<byte> responseBytes(
        reinterpret_cast<const std::byte*>(serializedResponse.data()),
        reinterpret_cast<const std::byte*>(serializedResponse.data()) + serializedResponse.size()
    );

    // Send the response message
    adtService->sendMessage({device}, responseBytes);
}

////////////////////////////////////////////////////////////////////////////////
/// Message handlers
////////////////////////////////////////////////////////////////////////////////

Result<shared_ptr<ISerializable>> bleConfigUpdateHandler(unique_ptr<IPlugMessageData> message) {
    // Cast the message to the correct type
    auto bleConfigMessage = dynamic_cast<SetBleConfig*>(message.get());
    if (bleConfigMessage == nullptr) {
        return Result<shared_ptr<ISerializable>>::createFailure("failed to cast message to SetBleConfig");
    }

    // Update the device name for the BLE advertiser if a device name is present in the message
    if (bleConfigMessage->deviceName.has_value()) {
        if(BleAdvertiser::setName(bleConfigMessage->deviceName.value())) {
            ESP_LOGI(TAG, "ble device name updated");
        } else {
            return Result<shared_ptr<ISerializable>>::createFailure("failed to update ble device name");
        }
    }

    // Update the BLE configuration
    config.get()->bleConfig->deviceName = bleConfigMessage->deviceName.value_or(config.get()->bleConfig->deviceName);

    // Commit the configuration to NVS
    Result result = config->writePlugConfig(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY, *config);
    if (!result.isSuccess()) {
        return Result<shared_ptr<ISerializable>>::createFailure("failed to configuration to storage");
    }

    return Result<shared_ptr<ISerializable>>::createSuccess(nullptr);
}

Result<shared_ptr<ISerializable>> setAcDimmerConfigHandler(unique_ptr<IPlugMessageData> message) {
    ESP_LOGI(TAG, "setting ac dimmer config");
    return Result<shared_ptr<ISerializable>>::createFailure("not implemented");
}

Result<shared_ptr<ISerializable>> setWifiConfigHandler(unique_ptr<IPlugMessageData> message) {
    return Result<shared_ptr<ISerializable>>::createFailure("not implemented");
    // ESP_LOGI(TAG, "setting wifi config");

    // // Cast the message to the correct type
    // auto wifiConfigMessage = dynamic_cast<SetWifiConfig*>(message.get());
    // if (wifiConfigMessage == nullptr) {
    //     ESP_LOGE(TAG, "failed to cast message to SetWifiConfig");
    //     return;
    // }

    // // Update the wifi configuration
    // config.get()->wifiConfig = make_shared<WifiConfig>(
    //     wifiConfigMessage->ssid,
    //     wifiConfigMessage->password
    // );

    // // Commit the configuration to NVS
    // config->writePlugConfig(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY, *config);

    // // Attempt to connect to wifi
    // attemptConnectToWifi(config.get()->wifiConfig);
}

Result<shared_ptr<ISerializable>> setMqttConfigHandler(unique_ptr<IPlugMessageData> message) {
    ESP_LOGI(TAG, "setting mqtt config");
    return Result<shared_ptr<ISerializable>>::createFailure("not implemented");
}

////////////////////////////////////////////////////////////////////////////////
/// Meta functions
////////////////////////////////////////////////////////////////////////////////

esp_err_t reset() {
    // Erase the plug configuration from NVS
    esp_err_t error = nvs_flash_erase();
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "failed to erase nvs, error code: %s", esp_err_to_name(error));
        return error;
    }

    // Reboot the device
    // FIXME: Enable this in prod on a pin interrupt
    // esp_restart();

    return ESP_OK;
}