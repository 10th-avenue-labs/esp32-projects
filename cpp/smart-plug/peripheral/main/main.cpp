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
#include <WifiConnectionInfo.h>
#include <CloudConnectionInfo.h>
#include <SetMqttConfig.h>
#include <SetBleConfig.h>
#include <DeviceInfo.h>

// Define the namespace and key for the plug configuration
#define PLUG_CONFIG_NAMESPACE "storage"
#define PLUG_CONFIG_KEY "config"

// BLE constants
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

// Define the ADT service and characteristic UUIDs
#define ADT_SERVICE_UUID "cc3aab60-0001-0000-81e0-c88f19fb28cb"
#define ADT_SERVICE_MTU_CHARACTERISTIC_UUID "cc3aab60-0001-0001-81e0-c88f19fb28cb"
#define ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID "cc3aab60-0001-0002-81e0-c88f19fb28cb"
#define ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID "cc3aab60-0001-0003-81e0-c88f19fb28cb"

static const char *TAG = "SMART_PLUG";

// Initialization helpers
Result<shared_ptr<PlugConfig>> getConfigOrDefault();
void initializeMqttClient(const string &brokerUri);

// ADT message handler
void adtMessageHandler(uint16_t messageId, vector<byte> message, shared_ptr<BleDevice> device);

// Message handlers
Result<shared_ptr<ISerializable>> getDeviceInfoHandler(unique_ptr<IPlugMessageData> message);
Result<shared_ptr<ISerializable>> bleConfigUpdateHandler(unique_ptr<IPlugMessageData> message);
Result<shared_ptr<ISerializable>> setAcDimmerConfigHandler(unique_ptr<IPlugMessageData> message);
Result<shared_ptr<ISerializable>> initiateCloudConnectionHandler(unique_ptr<IPlugMessageData> message);
Result<shared_ptr<ISerializable>> setMqttConfigHandler(unique_ptr<IPlugMessageData> message);

// Cloud connect methods
Result<> attemptCloudConnect(CloudConnectionInfo *cloudConnectionInfo, int timeoutMs);
Result<> attemptCloudConnectWithRetries(CloudConnectionInfo *cloudConnectionInfo, int timeoutMs);
void longTermCloudDisconnectHandler();
void initiateLongTermCloudConnecter();

// Meta functions
esp_err_t reset();

// Shared resources
shared_ptr<PlugConfig> config;
unique_ptr<Mqtt::MqttClient> mqttClient;
unique_ptr<AdtService> adtService;
unique_ptr<AcDimmer> acDimmer;
unique_ptr<int> cloudConnectionAttempt = make_unique<int>(0);

extern "C" void app_main(void)
{
    reset();

    // Register the Plug message deserializers
    PlugMessage::registerDeserializer("SetAcDimmerConfig", SetAcDimmerConfig::deserialize);
    PlugMessage::registerDeserializer("WifiConnectionInfo", WifiConnectionInfo::deserialize);
    PlugMessage::registerDeserializer("SetMqttConfig", SetMqttConfig::deserialize);
    PlugMessage::registerDeserializer("SetBleConfig", SetBleConfig::deserialize);
    PlugMessage::registerDeserializer("InitiateCloudConnection", CloudConnectionInfo::deserialize);

    // Get config or default
    Result<shared_ptr<PlugConfig>> plugConfigResult = getConfigOrDefault();
    if (!plugConfigResult.success)
    {
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
        config.get()->acDimmerConfig->brightness);

    // Create the ADT service
    ESP_LOGI(TAG, "creating adt service");
    adtService = make_unique<AdtService>(
        ADT_SERVICE_UUID,
        ADT_SERVICE_MTU_CHARACTERISTIC_UUID,
        ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID,
        ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID,
        adtMessageHandler);

    // Initialize the BLE advertiser
    ESP_LOGI(TAG, "initializing ble advertiser");
    BleAdvertiser::init(
        config.get()->bleConfig->deviceName,
        BLE_GAP_APPEARANCE_GENERIC_TAG,
        BLE_GAP_LE_ROLE_PERIPHERAL,
        {adtService->getBleService()},
        nullptr // No device connected handler needed
    );

    // Initialize the wifi service
    ESP_LOGI(TAG, "initializing wifi service");
    WifiService::WifiService::init();
    shared_ptr<int> wifiConnectionAttempt = make_shared<int>(0);
    shared_ptr<WifiConfig> wifiConfig = config.get()->wifiConfig;

    // Initialize the mqtt client
    ESP_LOGI(TAG, "initializing mqtt client");
    initializeMqttClient(config.get()->mqttConfig->brokerAddress);

    string serializedCloudConnectionInfo = "{ \
        \"deviceId\":\"1234\", \
        \"jwt\":\"123\", \
        \"wifiConnectionInfo\":{ \
            \"ssid\":\"IP-in-the-hot-tub\", \
            \"password\":\"everytim\" \
        }, \
        \"mqttConnectionString\":\"mqtt://10.11.2.80:188\" \
    }";
    const auto &info = CloudConnectionInfo::deserialize(serializedCloudConnectionInfo);
    auto cloudConnectionInfo = dynamic_cast<CloudConnectionInfo *>(info.get());
    ESP_LOGI(TAG, "Device ID: %s", cloudConnectionInfo->deviceId.c_str());
    ESP_LOGI(TAG, "JWT: %s", cloudConnectionInfo->jwt.c_str());
    ESP_LOGI(TAG, "SSID: %s", cloudConnectionInfo->wifiConnectionInfo.ssid.c_str());
    ESP_LOGI(TAG, "Password: %s", cloudConnectionInfo->wifiConnectionInfo.password.c_str());
    ESP_LOGI(TAG, "MQTT Connection String: %s", cloudConnectionInfo->mqttConnectionString.c_str());

    // Set the config to the cloud connection info
    config->wifiConfig = make_shared<WifiConfig>(cloudConnectionInfo->wifiConnectionInfo.ssid, cloudConnectionInfo->wifiConnectionInfo.password);
    ESP_LOGE(TAG, "EEK");
    config->mqttConfig = make_shared<MqttConfig>(cloudConnectionInfo->mqttConnectionString, cloudConnectionInfo->deviceId, cloudConnectionInfo->jwt);

    // // Connect to wifi
    // ESP_LOGI(TAG, "connecting to wifi");
    // bool succeeded = WifiService::startConnect({
    //     cloudConnectionInfo->wifiConnectionInfo.ssid,
    //     cloudConnectionInfo->wifiConnectionInfo.password,
    // });
    // bool specifiedStateReached = WifiService::waitConnectionState({ConnectionState::CONNECTED, ConnectionState::NOT_CONNECTED}, 10000);
    // ESP_LOGE(TAG, "specified state reached: %d", specifiedStateReached);
    // ESP_LOGE(TAG, "wifi connection state: %d", WifiService::getConnectionState());
    // return;

    // Initiate the long-term cloud connection
    initiateLongTermCloudConnecter();

    // Attempt to connect to cloud (this should fail)
    Result<> cloudConnectResult = attemptCloudConnect(cloudConnectionInfo, 10000);

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    return;

    // // Try and connect to wifi if a wifi configuration is present
    // if (config.get()->wifiConfig != nullptr)
    // {
    //     attemptConnectToWifi(config.get()->wifiConfig);
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "no wifi configuration found, skipping wifi connection");
    // }

    // Begin advertising
    ESP_LOGI(TAG, "beginning advertising");
    BleAdvertiser::advertise();
}

////////////////////////////////////////////////////////////////////////////////
/// MQTT Client Accessor
////////////////////////////////////////////////////////////////////////////////

void initializeMqttClient(const string &brokerUri)
{
    if (mqttClient)
    {
        return;
    }

    mqttClient = make_unique<Mqtt::MqttClient>(brokerUri);
}

////////////////////////////////////////////////////////////////////////////////
/// Initialization helpers
////////////////////////////////////////////////////////////////////////////////

Result<shared_ptr<PlugConfig>> getConfigOrDefault()
{
    // Read the plug configuration from NVS if it exists
    Result<shared_ptr<PlugConfig>> plugConfigResult = PlugConfig::readPlugConfig(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY);
    if (!plugConfigResult.isSuccess())
    {
        return plugConfigResult;
    }

    // Check if a configuration was found
    if (plugConfigResult.getValue() != nullptr)
    {
        ESP_LOGI(TAG, "found plug configuration");
        return plugConfigResult;
    }

    // Create a default plug configuration if one doesn't already exist
    // TODO: Consider defining the default function somewhere else
    ESP_LOGI(TAG, "no plug configuration found, creating a default one");
    config = make_unique<PlugConfig>(
        PlugConfig{
            // Config for BLE
            make_shared<BleConfig>(
                "Smart Plug"),
            // Config for non-dimmable LED
            make_shared<AcDimmerConfig>(
                32,
                25,
                1000,
                5000,
                1200,
                0),
            // Config for WiFi
            nullptr,
            // Config for MQTT
            make_shared<MqttConfig>(
                "mqtt://10.11.2.96:1883")});

    return Result<shared_ptr<PlugConfig>>::createSuccess(move(config));
}

////////////////////////////////////////////////////////////////////////////////
/// Cloud Connection Handlers
////////////////////////////////////////////////////////////////////////////////

enum ConnectionState
{
    NOT_CONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
};

static atomic<int> atomicValue{ConnectionState::NOT_CONNECTED};

// Attempt to connect the device to the cloud
Result<> connectCloud()
{
    // Note:
    // This needs to be thread-safe
    // Needs to handle partial connections

    // Atomically ensure that we're in the correct connection state (NOT_CONNECTED) and update the connection state to CONNECTING if so
    int expectedValue = ConnectionState::NOT_CONNECTED;
    if (!atomicValue.compare_exchange_strong(expectedValue, ConnectionState::CONNECTING))
    {
        return Result<>::createFailure(format("Failed to connect to cloud, device is already in a connection state of: %d", expectedValue));
    }

    // Check if we need to wait before attempting to connect again

    // Check if wifi is connected
    if (/* wifi not connected */ 1)
    {
        // Connect wifi
    }

    // TODO: What if we get a disconnect while connecting?

    // Check if mqtt is connected
    if (/* mqtt not connected */ 1)
    {
        // Connect mqtt
    }

    // Subscribe to the device's requested state topic
    // TODO: Ensure we can do this multiple times in a row without issue
}

void onWifiDisconnect()
{
    // Check the connection state
    // Not_connected - ConnectCloud
    // Connecting - Wait until connection finishes, then set state to Not_connected, then ConnectCloud
    // Connected - set state to Not_connected, then connect cloud
    // Disconnecting - Wait until disconnect finishes, then set state to Not_connected, then connect cloud
}

void onMqttDisconnect()
{
    // Check the connection state
    // Not_connected - ConnectCloud
    // Connecting - Wait until connection finishes, then set state to Not_connected, then ConnectCloud
    // Connected - set state to Not_connected, then connect cloud
    // Disconnecting - Wait until disconnect finishes, then set state to Not_connected, then connect cloud
}

void initiateConnectionLoop()
{
    // Reset connection into (attempts and whatnot)
    // Set the wifi and mqtt disconnect handlers
}

void setup()
{
    // ConnectCloud
    // fail? - return failure
    // Initiate Connection Loop
    // Save configuration
    // Return Success
}

void poweredOn()
{
    // Initiate connection loop
    // Connect cloud
}

/**
 * @brief Attempt to connect to the cloud. This will:
 * 1. Connect to wifi
 * 2. Connect to mqtt
 * 3. Subscribe to the device's requested state topic
 *
 * This does not handle saving to ephemeral config or NVS and also has no retry-logic
 *
 * @param cloudConnectionInfo The cloud connection info
 * @param timeoutMs The timeout in milliseconds
 * @return Result<> A result indicating success or failure
 */
Result<> attemptCloudConnect(CloudConnectionInfo *cloudConnectionInfo, int timeoutMs)
{
    // Log the cloud connection attempt
    ESP_LOGI(TAG, "attempting cloud connection with timeout %d ms", timeoutMs);

    // Attempt to connect to wifi
    bool connectionStarted = WifiService::WifiService::startConnect({
        cloudConnectionInfo->wifiConnectionInfo.ssid,
        cloudConnectionInfo->wifiConnectionInfo.password,
    });
    if (!connectionStarted)
    {
        return Result<>::createFailure("Failed to start connecting to wifi");
    }
    bool specifiedWifiStateReached = WifiService::WifiService::waitConnectionState({WifiService::ConnectionState::CONNECTED, WifiService::ConnectionState::NOT_CONNECTED}, timeoutMs);
    ESP_LOGE(TAG, "waited, wifi connection state: %d", WifiService::WifiService::getConnectionState());
    if (!specifiedWifiStateReached)
    {
        ESP_LOGE(TAG, "XXXXspecified state not reached");
        return Result<>::createFailure("Wifi connection timed out");
    }
    if (WifiService::WifiService::getConnectionState() != WifiService::ConnectionState::CONNECTED)
    {
        return Result<>::createFailure("Wifi connection failed");
    }
    ESP_LOGI(TAG, "connected to wifi");

    // Attempt to connect to mqtt
    mqttClient->setBrokerUri(cloudConnectionInfo->mqttConnectionString);
    mqttClient->connect();
    bool specifiedMqttStateReached = mqttClient->waitConnectionState({Mqtt::ConnectionState::CONNECTED, Mqtt::ConnectionState::DISCONNECTING}, timeoutMs); // TODO: This timeout should be subtracted from the elapsed time of this function so far ideally
    if (!specifiedMqttStateReached)
    {
        return Result<>::createFailure("Mqtt connection timed out");
    }
    if (mqttClient->getConnectionState() != Mqtt::ConnectionState::CONNECTED)
    {
        return Result<>::createFailure("Mqtt connection failed");
    }
    ESP_LOGI(TAG, "connected to mqtt");

    // Subscribe to topic
    int messageId = mqttClient->subscribe(format("device/{}/requested_state", cloudConnectionInfo->deviceId), [](Mqtt::MqttClient *client, int msgId, vector<byte> message)
                                          { ESP_LOGI(TAG, "received message on topic 'smartplug'"); });
    if (messageId == -1)
    {
        return Result<>::createFailure("Failed to subscribe to topic");
    }

    return Result<>::createSuccess();
}

/**
 * @brief Attempt to connect to the cloud with retries. This will:
 * 1. Connect to wifi
 * 2. Connect to mqtt
 * 3. Subscribe to the device's requested state topic
 * 4. Retry the above steps until success or the timeout is reached
 *
 * @param cloudConnectionInfo The cloud connection info
 * @param timeoutMs The timeout in milliseconds
 * @return Result<> A result indicating success or failure
 */
Result<> attemptCloudConnectWithRetries(CloudConnectionInfo *cloudConnectionInfo, int timeoutMs)
{
    // Get the current time
    struct timeval timeval;
    gettimeofday(&timeval, NULL);
    uint64_t currentTimeMs = timeval.tv_sec * 1000 + (timeval.tv_usec / 1000);
    ESP_LOGI(TAG, "current time: %" PRIu64, currentTimeMs);

    // Get the end time
    uint64_t endTimeMs = currentTimeMs + timeoutMs;

    // Attempt to connect to cloud with retries
    Result<> result = Result<>::createFailure("Failed attempting to connect to cloud");
    while (currentTimeMs < endTimeMs)
    {
        // Attempt to connect to cloud
        result = attemptCloudConnect(cloudConnectionInfo, endTimeMs - currentTimeMs);
        if (result.isSuccess())
        {
            break;
        }
        ESP_LOGE(TAG, "failed to connect to cloud, retrying, error: %s", result.getError().c_str());

        // Update the current time
        gettimeofday(&timeval, NULL);
        currentTimeMs = timeval.tv_sec * 1000 + (timeval.tv_usec / 1000);
    }

    return result;
}

void longTermCloudDisconnectHandler()
{
    // Clear the wifi and mqtt onDisconnected handlers
    WifiService::WifiService::onDisconnect = nullptr;
    mqttClient->onDisconnected = nullptr;

    ESP_LOGE(TAG, "cloud disconnected handler");
    // Calculate the amount of time to wait before attempting to reconnect
    int waitSeconds = pow(2, *cloudConnectionAttempt);

    // Wait for the specified duration
    ESP_LOGW(TAG, "cloud disconnected on connection attempt %d, waiting %d seconds before reconnecting", *cloudConnectionAttempt, waitSeconds);
    vTaskDelay(pdMS_TO_TICKS(waitSeconds * 1000));

    // Create the wifo connection info from the configuration
    WifiConnectionInfo wifiConnectionInfo;
    wifiConnectionInfo.ssid = config.get()->wifiConfig->ssid;
    wifiConnectionInfo.password = config.get()->wifiConfig->password;

    // Create the cloud connection info from the configuration
    CloudConnectionInfo cloudConnectionInfo;
    cloudConnectionInfo.deviceId = config.get()->mqttConfig->deviceId;
    cloudConnectionInfo.jwt = config.get()->mqttConfig->jwt;
    cloudConnectionInfo.wifiConnectionInfo = wifiConnectionInfo;
    cloudConnectionInfo.mqttConnectionString = config.get()->mqttConfig->brokerAddress;

    // Attempt to reconnect to cloud
    Result result = attemptCloudConnect(&cloudConnectionInfo, 30000);
    if (result.isSuccess())
    {
        // Update the connection attempt
        *cloudConnectionAttempt = 0;
        return;
    }
    ESP_LOGE(TAG, "lt handler failed to reconnect to cloud, error: %s", result.getError().c_str());

    // If the wait seconds is already larger than 1800 (30 minutes), do not increase further.
    if (waitSeconds > 1800)
    {
        return;
    }

    (*cloudConnectionAttempt)++;
}

void initiateLongTermCloudConnecter()
{
    // Set the onDisconnected handlers for wifi and mqtt
    WifiService::WifiService::onDisconnect = []()
    {
        ESP_LOGE(TAG, "wifi disconnected");
        longTermCloudDisconnectHandler();
    };
    mqttClient->onDisconnected = [](Mqtt::MqttClient *client)
    {
        ESP_LOGE(TAG, "MQTT disconnected");
        longTermCloudDisconnectHandler();
    };
}

////////////////////////////////////////////////////////////////////////////////
/// Adt message handler
////////////////////////////////////////////////////////////////////////////////

void adtMessageHandler(uint16_t messageId, vector<byte> message, shared_ptr<BleDevice> device)
{
    // Convert the data to a string
    string messageString(
        reinterpret_cast<const char *>(message.data()),
        reinterpret_cast<const char *>(message.data()) + message.size());

    // Deserialize the message
    PlugMessage plugMessage = PlugMessage::deserialize(messageString);

    // Create a map of message handlers
    unordered_map<string, function<Result<shared_ptr<ISerializable>>(unique_ptr<IPlugMessageData>)>> messageHandlers = {
        {"GetDeviceInfo", getDeviceInfoHandler},
        {"SetBleConfig", bleConfigUpdateHandler},
        {"InitiateCloudConnection", initiateCloudConnectionHandler},
        {"SetAcDimmerConfig", setAcDimmerConfigHandler},
        {"SetMqttConfig", setMqttConfigHandler}};

    // Get the message handler
    auto messageHandler = messageHandlers.find(plugMessage.type);

    // Check if the message handler was found
    if (messageHandler == messageHandlers.end())
    {
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
        reinterpret_cast<const std::byte *>(serializedResponse.data()),
        reinterpret_cast<const std::byte *>(serializedResponse.data()) + serializedResponse.size());

    // Send the response message
    adtService->sendMessage({device}, responseBytes);
}

////////////////////////////////////////////////////////////////////////////////
/// Message handlers
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Handler for the getDeviceInfo message.
 *
 * @param message The message data
 * @return Result<shared_ptr<ISerializable>> A result containing the device info
 */
Result<shared_ptr<ISerializable>> getDeviceInfoHandler(unique_ptr<IPlugMessageData> message)
{
    // Ensure the message is null (device info request messages have no data)
    if (message != nullptr)
    {
        return Result<shared_ptr<ISerializable>>::createFailure("GetDeviceInfo message data is not null");
    }

    // Create a device info object
    shared_ptr<DeviceInfo> deviceInfo = make_shared<DeviceInfo>(
        DeviceInfo(
            "SmartPlug", // FIXME: This is the wrong name
            "SmartPlug"));

    // Return a result with the device info
    return Result<shared_ptr<ISerializable>>::createSuccess(deviceInfo);
}

/**
 * @brief Handler for the SetBleConfig message.
 *
 * @param message The message data
 * @return Result<shared_ptr<ISerializable>> A result indicating success or failure
 */
Result<shared_ptr<ISerializable>> bleConfigUpdateHandler(unique_ptr<IPlugMessageData> message)
{
    // Cast the message to the correct type
    auto bleConfigMessage = dynamic_cast<SetBleConfig *>(message.get());
    if (bleConfigMessage == nullptr)
    {
        return Result<shared_ptr<ISerializable>>::createFailure("Failed to cast message to SetBleConfig");
    }

    // Update the device name for the BLE advertiser if a device name is present in the message
    if (bleConfigMessage->deviceName.has_value())
    {
        if (BleAdvertiser::setName(bleConfigMessage->deviceName.value()))
        {
            ESP_LOGI(TAG, "ble device name updated to '%s'", bleConfigMessage->deviceName.value().c_str());
        }
        else
        {
            return Result<shared_ptr<ISerializable>>::createFailure("Failed to update ble device name");
        }
    }

    // Update the BLE configuration
    config.get()->bleConfig->deviceName = bleConfigMessage->deviceName.value_or(config.get()->bleConfig->deviceName);

    // Commit the configuration to NVS
    Result result = config->writePlugConfig(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY, *config);
    if (!result.isSuccess())
    {
        return Result<shared_ptr<ISerializable>>::createFailure("Failed to configuration to storage");
    }

    return Result<shared_ptr<ISerializable>>::createSuccess(nullptr);
}

Result<shared_ptr<ISerializable>> initiateCloudConnectionHandler(unique_ptr<IPlugMessageData> message)
{
    // Cast the message to the correct type
    auto cloudConnectionInfo = dynamic_cast<CloudConnectionInfo *>(message.get());
    if (cloudConnectionInfo == nullptr)
    {
        return Result<shared_ptr<ISerializable>>::createFailure("Failed to cast message to InitiateCloudConnection");
    }

    // Attempt to connect to the cloud
    Result<> result = attemptCloudConnectWithRetries(cloudConnectionInfo, 30000);
    if (!result.isSuccess())
    {
        // TODO: Consider disconnecting the mqtt client and wifi service
        return Result<shared_ptr<ISerializable>>::createFailure(format("Failed to connect to cloud: {}", result.getError()));
    }

    // Update the configuration
    config.get()->wifiConfig = make_shared<WifiConfig>(
        cloudConnectionInfo->wifiConnectionInfo.ssid,
        cloudConnectionInfo->wifiConnectionInfo.password);
    config.get()->mqttConfig = make_shared<MqttConfig>(
        cloudConnectionInfo->mqttConnectionString,
        cloudConnectionInfo->deviceId,
        cloudConnectionInfo->jwt);

    // Save the configuration to NVS
    Result<> saveResult = config->writePlugConfig(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY, *config);
    if (!saveResult.isSuccess())
    {
        return Result<shared_ptr<ISerializable>>::createFailure("Failed to save configuration to storage");
    }

    // Seup the long-term reconnection logic
    initiateLongTermCloudConnecter();

    return Result<shared_ptr<ISerializable>>::createSuccess(nullptr);
}

Result<shared_ptr<ISerializable>> setAcDimmerConfigHandler(unique_ptr<IPlugMessageData> message)
{
    ESP_LOGI(TAG, "setting ac dimmer config");
    return Result<shared_ptr<ISerializable>>::createFailure("not implemented");
}

Result<shared_ptr<ISerializable>> setMqttConfigHandler(unique_ptr<IPlugMessageData> message)
{
    ESP_LOGI(TAG, "setting mqtt config");
    return Result<shared_ptr<ISerializable>>::createFailure("not implemented");
}

////////////////////////////////////////////////////////////////////////////////
/// Meta functions
////////////////////////////////////////////////////////////////////////////////

esp_err_t reset()
{
    // Erase the plug configuration from NVS
    esp_err_t error = nvs_flash_erase();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to erase nvs, error code: %s", esp_err_to_name(error));
        return error;
    }

    // Reboot the device
    // FIXME: Enable this in prod on a pin interrupt
    // esp_restart();

    return ESP_OK;
}