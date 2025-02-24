#include "SmartDevice.h"
#include "Result.h"
#include "./include/TestMessage.h"
#include "./include/TestDeviceConfig.h"

#include <iostream>
#include <string>
#include "esp_log.h"
#include "esp_event.h"
#include "include/TestDevice.h"

using namespace std;

static const char *TAG = "MAIN";

extern "C" void app_main()
{
    /**
     * Things needed for a type of smart device:
     * 1. [DONE] Config applicable to just the smart device type. i.e. an ac dimmer would have a config to store properties like the brightness value
     * 2. [DONE] Event handlers for the smart device type. i.e. an ac dimmer would have an event handler for the brightness value changing
     * 3. [DONE] Serializers and deserializers for the config and message types. There's a lot of overlap here, but not necessarily 100% overlap
     * 4. Potential overrides of functions? Maybe like the init to initialize an ac dimmer?
     */

    // Register deserializers
    IDeserializable::registerDeserializer<SmartDevice::SmartDeviceConfig>(SmartDevice::SmartDeviceConfig::deserialize);
    IDeserializable::registerDeserializer<SmartDevice::BleConfig>(SmartDevice::BleConfig::deserialize);
    IDeserializable::registerDeserializer<SmartDevice::CloudConnectionConfig>(SmartDevice::CloudConnectionConfig::deserialize);
    IDeserializable::registerDeserializer<TestDeviceConfig>(TestDeviceConfig::deserialize);
    IDeserializable::registerDeserializer<SmartDevice::Message>(SmartDevice::Message::deserialize);
    IDeserializable::registerDeserializer<TestMessage>(TestMessage::deserialize);

    ///////////////////////////////////////////////////////////////////////////
    // Test code for serialization and deserialization
    ///////////////////////////////////////////////////////////////////////////

    // Create a SmartDeviceConfig object
    SmartDevice::SmartDeviceConfig config(
        make_unique<SmartDevice::BleConfig>("My Device"),
        make_unique<SmartDevice::CloudConnectionConfig>(
            "deviceId",
            "jwt",
            "ssid",
            "password",
            "mqttConnectionString"));

    // Serialize the config
    auto serializedConfig = config.serializeToString(true);
    ESP_LOGI(TAG, "serialized config: %s", serializedConfig.c_str());

    // Deserialize the config
    Result<std::unique_ptr<IDeserializable>> deserializedConfigResult = SmartDevice::SmartDeviceConfig::deserialize(config.serialize().get());
    if (!deserializedConfigResult.isSuccess())
    {
        ESP_LOGE(TAG, "Failed to deserialize config: %s", deserializedConfigResult.getError().c_str());
        return;
    }
    auto deserializedConfig = std::unique_ptr<SmartDevice::SmartDeviceConfig>(dynamic_cast<SmartDevice::SmartDeviceConfig *>(deserializedConfigResult.getValue().release()));
    ESP_LOGI(TAG, "deserialized config, BLE device name: %s", deserializedConfig->bleConfig->deviceName.c_str());
    ESP_LOGI(TAG, "deserialized config, Cloud info deviceId: %s", deserializedConfig->cloudConnectionConfig->deviceId.c_str());
    ESP_LOGI(TAG, "deserialized config, Cloud info jwt: %s", deserializedConfig->cloudConnectionConfig->jwt.c_str());
    ESP_LOGI(TAG, "deserialized config, Cloud info ssid: %s", deserializedConfig->cloudConnectionConfig->ssid.c_str());
    ESP_LOGI(TAG, "deserialized config, Cloud info password: %s", deserializedConfig->cloudConnectionConfig->password.c_str());
    ESP_LOGI(TAG, "deserialized config, Cloud info mqttConnectionString: %s", deserializedConfig->cloudConnectionConfig->mqttConnectionString.c_str());

    // Create a TestDeviceConfig object
    TestDeviceConfig testConfig(
        42,
        3.14,
        true,
        "Hello, world!",
        {1, 2, 3, 4, 5},
        make_unique<SmartDevice::BleConfig>("My New Device"),
        make_unique<SmartDevice::CloudConnectionConfig>(
            "deviceId",
            "jwt",
            "ssid",
            "password",
            "mqttConnectionString"));

    // Serialize the test config
    auto serializedTestConfig = testConfig.serializeToString(true);
    ESP_LOGI(TAG, "serialized test config: %s", serializedTestConfig.c_str());

    // Deserialize the test config
    auto deserializedTestConfigResult = TestDeviceConfig::deserialize(testConfig.serialize().get());
    if (!deserializedTestConfigResult.isSuccess())
    {
        ESP_LOGE(TAG, "Failed to deserialize test config: %s", deserializedTestConfigResult.getError().c_str());
        return;
    }
    auto deserializedTestConfig = std::unique_ptr<TestDeviceConfig>(dynamic_cast<TestDeviceConfig *>(deserializedTestConfigResult.getValue().release()));
    ESP_LOGI(TAG, "deserialized test config deviceName: %s", deserializedTestConfig->bleConfig->deviceName.c_str());
    ESP_LOGI(TAG, "deserialized test config myTestInt: %d", deserializedTestConfig->myTestInt);
    ESP_LOGI(TAG, "deserialized test config myTestFloat: %f", deserializedTestConfig->myTestFloat);
    ESP_LOGI(TAG, "deserialized test config myTestBool: %s", deserializedTestConfig->myTestBool ? "true" : "false");
    ESP_LOGI(TAG, "deserialized test config myTestString: %s", deserializedTestConfig->myTestString.c_str());
    ESP_LOGI(TAG, "deserialized test config myTestVector:");
    for (int i = 0; i < deserializedTestConfig->myTestVector.size(); i++)
    {
        ESP_LOGI(TAG, "  %d", deserializedTestConfig->myTestVector[i]);
    }

    // Create a TestMessage
    string testMessageString = R"({
        "myInt": 42,
        "myFloat": 3.14,
        "myBool": true,
        "myString": "Hello, world!",
        "myVector": [1, 2, 3, 4, 5]
    })";
    ESP_LOGI(TAG, "Test message string: %s", testMessageString.c_str());

    // Deserialize the TestMessage object
    auto testMessageDeserializer = IDeserializable::getDeserializer<TestMessage>();
    auto deserializedTestMessageResult = testMessageDeserializer(cJSON_Parse(testMessageString.c_str()));
    if (!deserializedTestMessageResult.isSuccess())
    {
        ESP_LOGE(TAG, "Failed to deserialize test message: %s", deserializedTestMessageResult.getError().c_str());
        return;
    }
    auto deserializedTestMessage = std::unique_ptr<TestMessage>(dynamic_cast<TestMessage *>(deserializedTestMessageResult.getValue().release()));
    ESP_LOGI(TAG, "deserialized test message myInt: %d", deserializedTestMessage->myInt);
    ESP_LOGI(TAG, "deserialized test message myFloat: %f", deserializedTestMessage->myFloat);
    ESP_LOGI(TAG, "deserialized test message myBool: %s", deserializedTestMessage->myBool ? "true" : "false");
    ESP_LOGI(TAG, "deserialized test message myString: %s", deserializedTestMessage->myString.c_str());
    ESP_LOGI(TAG, "deserialized test message myVector:");
    for (int i = 0; i < deserializedTestMessage->myVector.size(); i++)
    {
        ESP_LOGI(TAG, "  %d", deserializedTestMessage->myVector[i]);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Test code for the smart device event handling
    ///////////////////////////////////////////////////////////////////////////

    ESP_LOGI(TAG, "\n\ntest code for the smart device event handling\n\n");

    // Create a TestSmartDevice object
    TestDevice device(std::move(*deserializedTestConfig));

    // Create a test payload
    string testPayload = R"({
        "type": "testMessage",
        "data": {
            "myInt": 42,
            "myFloat": 3.14,
            "myBool": true,
            "myString": "Hello, world!",
            "myVector": [1, 2, 3, 4, 5]
        }
    })";
    ESP_LOGI(TAG, "test payload: %s", testPayload.c_str());

    // Handle the test message
    auto result = device.handleMessage(testPayload);
    if (!result.isSuccess())
    {
        ESP_LOGE(TAG, "failed to handle message: %s", result.getError().c_str());
        return;
    }
    // Serialize the response
    auto response = result.getValue();
    auto serializedResponse = response->serializeToString(true);
    ESP_LOGI(TAG, "serialized response: %s", serializedResponse.c_str());

    ///////////////////////////////////////////////////////////////////////////
    // Test code for initiation
    ///////////////////////////////////////////////////////////////////////////
}