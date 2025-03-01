#include "SmartDevice.h"
#include "Result.h"
#include "./include/TestRequest.h"
#include "./include/TestDeviceConfig.h"

#include <iostream>
#include <string>
#include "esp_log.h"
#include "esp_event.h"
#include "include/TestDevice.h"

using namespace std;

static const char *TAG = "MAIN";

/**
 * @brief Tests related to serialization and deserialization of configs
 *
 */
void testConfigSerializationAndDeserialization()
{
    ESP_LOGI(TAG, "\n\ntest code for serialization and deserialization of configs\n\n");

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
    /*
    {
        "bleConfig": {
            "deviceName": "My Device"
        },
        "cloudConnectionConfig": {
            "deviceId": "deviceId",
            "jwt": "jwt",
            "ssid": "ssid",
            "password": "password",
            "mqttConnectionString": "mqttConnectionString"
        }
    }
    */

    // Deserialize the config
    Result<std::unique_ptr<IDeserializable>> deserializedConfigResult = SmartDevice::SmartDeviceConfig::deserialize(config.serialize().get());
    if (!deserializedConfigResult.isSuccess())
    {
        ESP_LOGE(TAG, "Failed to deserialize config: %s", deserializedConfigResult.getError().c_str());
        return;
    }
    auto deserializedConfig = std::unique_ptr<SmartDevice::SmartDeviceConfig>(dynamic_cast<SmartDevice::SmartDeviceConfig *>(deserializedConfigResult.getValue().release()));
    ESP_LOGI(TAG, "deserialized config, BLE device name: %s", deserializedConfig->bleConfig->deviceName.c_str());                                       // My Device
    ESP_LOGI(TAG, "deserialized config, Cloud info deviceId: %s", deserializedConfig->cloudConnectionConfig->deviceId.c_str());                         // deviceId
    ESP_LOGI(TAG, "deserialized config, Cloud info jwt: %s", deserializedConfig->cloudConnectionConfig->jwt.c_str());                                   // jwt
    ESP_LOGI(TAG, "deserialized config, Cloud info ssid: %s", deserializedConfig->cloudConnectionConfig->ssid.c_str());                                 // ssid
    ESP_LOGI(TAG, "deserialized config, Cloud info password: %s", deserializedConfig->cloudConnectionConfig->password.c_str());                         // password
    ESP_LOGI(TAG, "deserialized config, Cloud info mqttConnectionString: %s", deserializedConfig->cloudConnectionConfig->mqttConnectionString.c_str()); // mqttConnectionString

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
    /*
    {
        "myTestInt": 42,
        "myTestFloat": 3.1400001049041748,
        "myTestBool": true,
        "myTestString": "Hello, world!",
        "myTestVector": [1, 2, 3, 4, 5],
        "bleConfig": {
            "deviceName": "My New Device"
        },
        "cloudConnectionConfig": {
            "deviceId": "deviceId",
            "jwt": "jwt",
            "ssid": "ssid",
            "password": "password",
            "mqttConnectionString": "mqttConnectionString"
        }
    }
    */

    // Deserialize the test config
    auto deserializedTestConfigResult = TestDeviceConfig::deserialize(testConfig.serialize().get());
    if (!deserializedTestConfigResult.isSuccess())
    {
        ESP_LOGE(TAG, "Failed to deserialize test config: %s", deserializedTestConfigResult.getError().c_str());
        return;
    }
    auto deserializedTestConfig = std::unique_ptr<TestDeviceConfig>(dynamic_cast<TestDeviceConfig *>(deserializedTestConfigResult.getValue().release()));
    ESP_LOGI(TAG, "deserialized test config deviceName: %s", deserializedTestConfig->bleConfig->deviceName.c_str()); // My New Device
    ESP_LOGI(TAG, "deserialized test config myTestInt: %d", deserializedTestConfig->myTestInt);                      // 42
    ESP_LOGI(TAG, "deserialized test config myTestFloat: %f", deserializedTestConfig->myTestFloat);                  // 3.14
    ESP_LOGI(TAG, "deserialized test config myTestBool: %s", deserializedTestConfig->myTestBool ? "true" : "false"); // true
    ESP_LOGI(TAG, "deserialized test config myTestString: %s", deserializedTestConfig->myTestString.c_str());        // Hello, world!
    ESP_LOGI(TAG, "deserialized test config myTestVector:");                                                         // 1, 2, 3, 4, 5
    for (int i = 0; i < deserializedTestConfig->myTestVector.size(); i++)
    {
        ESP_LOGI(TAG, "  %d", deserializedTestConfig->myTestVector[i]);
    }
}

/**
 * @brief Tests related to deserialization of requests
 *
 */
void testRequestDeserialization()
{
    ESP_LOGI(TAG, "\n\ntest code for deserialization of requests\n\n");

    // Create a TestRequest
    string testRequestString = R"({
        "myInt": 42,
        "myFloat": 3.14,
        "myBool": true,
        "myString": "Hello, world!",
        "myVector": [1, 2, 3, 4, 5]
    })";
    ESP_LOGI(TAG, "Test request string: %s", testRequestString.c_str());
    /*
    {
        "myInt": 42,
        "myFloat": 3.1400001049041748,
        "myBool": true,
        "myString": "Hello, world!",
        "myVector": [1, 2, 3, 4, 5]
    }
    */

    // Deserialize the TestRequest object
    auto testRequestDeserializer = IDeserializable::getDeserializer<TestRequest>();
    auto deserializedTestRequestResult = testRequestDeserializer(cJSON_Parse(testRequestString.c_str()));
    if (!deserializedTestRequestResult.isSuccess())
    {
        ESP_LOGE(TAG, "Failed to deserialize test request: %s", deserializedTestRequestResult.getError().c_str());
        return;
    }
    auto deserializedTestRequest = std::unique_ptr<TestRequest>(dynamic_cast<TestRequest *>(deserializedTestRequestResult.getValue().release()));
    ESP_LOGI(TAG, "deserialized test request myInt: %d", deserializedTestRequest->myInt);                      // 42
    ESP_LOGI(TAG, "deserialized test request myFloat: %f", deserializedTestRequest->myFloat);                  // 3.14
    ESP_LOGI(TAG, "deserialized test request myBool: %s", deserializedTestRequest->myBool ? "true" : "false"); // true
    ESP_LOGI(TAG, "deserialized test request myString: %s", deserializedTestRequest->myString.c_str());        // Hello, world!
    ESP_LOGI(TAG, "deserialized test request myVector:");                                                      // 1, 2, 3, 4, 5
    for (int i = 0; i < deserializedTestRequest->myVector.size(); i++)
    {
        ESP_LOGI(TAG, "  %d", deserializedTestRequest->myVector[i]);
    }

    // Create a test request message payload
    string testRequestPayload = R"({
        "type": "testRequest",
        "data": {
            "myInt": 42,
            "myFloat": 3.14,
            "myBool": true,
            "myString": "Hello, world!",
            "myVector": [1, 2, 3, 4, 5]
        }
    })";
    ESP_LOGI(TAG, "Test request payload: %s", testRequestPayload.c_str());
    /*
    {
        "type": "testRequest",
        "data": {
            "myInt": 42,
            "myFloat": 3.1400001049041748,
            "myBool": true,
            "myString": "Hello, world!",
            "myVector": [1, 2, 3, 4, 5]
        }
    }
    */

    // Get the deserializer for the test request payload
    auto testRequestPayloadDeserializer = IDeserializable::getDeserializer<SmartDevice::Request>();
    if (!testRequestPayloadDeserializer)
    {
        ESP_LOGE(TAG, "Failed to get deserializer for test request payload");
        return;
    }

    // Deserialize the test request payload
    auto deserializedTestRequestPayloadResult = testRequestPayloadDeserializer(cJSON_Parse(testRequestPayload.c_str()));
    if (!deserializedTestRequestPayloadResult.isSuccess())
    {
        ESP_LOGE(TAG, "Failed to deserialize test request payload: %s", deserializedTestRequestPayloadResult.getError().c_str());
        return;
    }

    // Transfer ownership from the result and cast the request
    std::unique_ptr<SmartDevice::Request> deserializedRequest = std::unique_ptr<SmartDevice::Request>(dynamic_cast<SmartDevice::Request *>(deserializedTestRequestPayloadResult.getValue().release()));
    ESP_LOGI(SMART_DEVICE_TAG, "request type: %s", deserializedRequest->type.c_str());

    std::unique_ptr<TestRequest> deserializedTestRequestPayload = std::unique_ptr<TestRequest>(dynamic_cast<TestRequest *>(deserializedRequest->data.release()));
    ESP_LOGI(TAG, "deserialized test request payload myInt: %d", deserializedTestRequestPayload->myInt);                      // 42
    ESP_LOGI(TAG, "deserialized test request payload myFloat: %f", deserializedTestRequestPayload->myFloat);                  // 3.14
    ESP_LOGI(TAG, "deserialized test request payload myBool: %s", deserializedTestRequestPayload->myBool ? "true" : "false"); // true
    ESP_LOGI(TAG, "deserialized test request payload myString: %s", deserializedTestRequestPayload->myString.c_str());        // Hello, world!
    ESP_LOGI(TAG, "deserialized test request payload myVector:");                                                             // 1, 2, 3, 4, 5
    for (int i = 0; i < deserializedTestRequestPayload->myVector.size(); i++)
    {
        ESP_LOGI(TAG, "  %d", deserializedTestRequestPayload->myVector[i]);
    }
}

/**
 * @brief Tests related to serialization of responses
 *
 */
void testResponseSerialization()
{
    ESP_LOGI(TAG, "\n\ntest code for serialization of responses\n\n");

    // Create a TestResponse object
    TestResponse testResponse(42, 3.14, true, "Hello, world!", {1, 2, 3, 4, 5});

    // Serialize the response
    auto serializedResponse = testResponse.serializeToString(true);
    ESP_LOGI(TAG, "serialized response: %s", serializedResponse.c_str());
    /*
    {
        "myInt": 42,
        "myFloat": 3.1400001049041748,
        "myBool": true,
        "myString": "Hello, world!",
        "myVector": [1, 2, 3, 4, 5]
    }
    */

    // Create a response object
    SmartDevice::Response response("Response", make_unique<TestResponse>(42, 3.14, true, "Hello, world!", std::vector<int>{1, 2, 3, 4, 5}));

    // Serialize the message response
    auto serializedMessageResponse = response.serializeToString(true);
    ESP_LOGI(TAG, "serialized message response: %s", serializedMessageResponse.c_str());
    /*
    {
        "type": "Response",
        "data": {
            "myInt": 42,
            "myFloat": 3.1400001049041748,
            "myBool": true,
            "myString": "Hello, world!",
            "myVector": [1, 2, 3, 4, 5]
        }
    }
    */

    // Create a ResultResponderResponse response
    SmartDevice::Response responseWithResultResponderResponse(
        "Response",
        make_unique<SmartDevice::ResultResponderResponse>(
            101,
            Result<std::shared_ptr<ISerializable>>::createSuccess(make_shared<TestResponse>(
                42,
                3.14,
                true,
                "Hello, world!",
                std::vector<int>{1, 2, 3, 4, 5}))));

    // Serialize the message response
    auto serializedMessageResponseWithResultResponderResponse = responseWithResultResponderResponse.serializeToString(true);
    ESP_LOGI(TAG, "serialized message response with result responder response: %s", serializedMessageResponseWithResultResponderResponse.c_str());
    /*
    {
        "type": "Response",
        "data": {
            "messageId": 101,
            "result": {
                "success": true,
                "error": null,
                "value": {
                    "myInt": 42,
                    "myFloat": 3.1400001049041748,
                    "myBool": true,
                    "myString": "Hello, world!",
                    "myVector": [1, 2, 3, 4, 5]
                }
            }
        }
    }
    */

    // Create a ResultResponderResponse response with a failure
    SmartDevice::Response responseWithResultResponderResponseFailure(
        "Response",
        make_unique<SmartDevice::ResultResponderResponse>(
            101,
            Result<std::shared_ptr<ISerializable>>::createFailure("Failed to handle request")));

    // Serialize the message response
    auto serializedMessageResponseWithResultResponderResponseFailure = responseWithResultResponderResponseFailure.serializeToString(true);
    ESP_LOGI(TAG, "serialized message response with result responder response failure: %s", serializedMessageResponseWithResultResponderResponseFailure.c_str());
    /*
    {
        "type": "Response",
        "data": {
            "messageId": 101,
            "result": {
                "success": false,
                "error": "Failed to handle request",
                "value": null
            }
        }
    }
    */

    // Create a ResultResponderResponse response with a void success result
    auto responseWithResultResponderResponseVoidSuccess = SmartDevice::Response(
        "Response",
        make_unique<SmartDevice::ResultResponderResponse>(
            101,
            Result<std::shared_ptr<ISerializable>>::createSuccess(nullptr)));

    // Serialize the message response
    auto serializedMessageResponseWithResultResponderResponseVoidSuccess = responseWithResultResponderResponseVoidSuccess.serializeToString(true);
    ESP_LOGI(TAG, "serialized message response with result responder response void success: %s", serializedMessageResponseWithResultResponderResponseVoidSuccess.c_str());
    /*
    {
        "type": "Response",
        "data": {
            "messageId": 101,
            "result": {
                "success": true,
                "error": null,
                "value": null
            }
        }
    }
    */
}

/**
 * @brief Tests related to the handling of events
 *
 */
void testEventHandleing()
{
    ESP_LOGI(TAG, "\n\ntest code for the smart device event handling\n\n");

    // Create a Test Config
    TestDeviceConfig testConfig(
        42,
        3.14,
        true,
        "Hello, world!",
        {1, 2, 3, 4, 5},
        make_unique<SmartDevice ::BleConfig>("Test Device"),
        make_unique<SmartDevice ::CloudConnectionConfig>(
            "deviceId",
            "jwt",
            "IP-in-the-hot-tub",
            "everytime",
            "mqttConnectionString"));

    // Create a TestSmartDevice object
    TestDevice device(std::move(testConfig));

    // Create a test payload
    string testPayload = R"({
        "type": "testRequest",
        "data": {
            "myInt": 42,
            "myFloat": 3.14,
            "myBool": true,
            "myString": "Hello, world!",
            "myVector": [1, 2, 3, 4, 5]
        }
    })";
    ESP_LOGI(TAG, "test payload: %s", testPayload.c_str());
    /*
    {
        "type": "testRequest",
        "data": {
            "myInt": 42,
            "myFloat": 3.1400001049041748,
            "myBool": true,
            "myString": "Hello, world!",
            "myVector": [1, 2, 3, 4, 5]
        }
    }
    */

    // // Handle the test request (Need to make method public to test)
    // auto result = device.handleRequest(testPayload);
    // if (!result.isSuccess())
    // {
    //     ESP_LOGE(TAG, "failed to handle request: %s", result.getError().c_str());
    //     return;
    // }

    // // Serialize the response
    // auto handleResult = result.getValue();
    // if (!handleResult.isSuccess())
    // {
    //     ESP_LOGE(TAG, "failed to handle request with error for consumer: %s", handleResult.getError().c_str());
    //     return;
    // }

    // // Serialize the response
    // auto serializedResponse = handleResult.getValue()->serializeToString(true);
    // ESP_LOGI(TAG, "serialized response: %s", serializedResponse.c_str());
}

extern "C" void app_main()
{
    /**
     * Things needed for a type of smart device:
     * 1. [DONE] Config applicable to just the smart device type. i.e. an ac dimmer would have a config to store properties like the brightness value
     * 2. [DONE] Event handlers for the smart device type. i.e. an ac dimmer would have an event handler for the brightness value changing
     * 3. [DONE] Serializers and deserializers for the config and request types. There's a lot of overlap here, but not necessarily 100% overlap
     * 4. Potential overrides of functions? Maybe like the init to initialize an ac dimmer?
     */

    // Register deserializers
    IDeserializable::registerDeserializer<SmartDevice::SmartDeviceConfig>(SmartDevice::SmartDeviceConfig::deserialize);
    IDeserializable::registerDeserializer<SmartDevice::BleConfig>(SmartDevice::BleConfig::deserialize);
    IDeserializable::registerDeserializer<SmartDevice::CloudConnectionConfig>(SmartDevice::CloudConnectionConfig::deserialize);
    IDeserializable::registerDeserializer<TestDeviceConfig>(TestDeviceConfig::deserialize);
    IDeserializable::registerDeserializer<SmartDevice::Request>(SmartDevice::Request::deserialize);
    IDeserializable::registerDeserializer<TestRequest>(TestRequest::deserialize);

    // Register the request types
    // NOTE: This is only done for testing here. The actual registration of request types is taken care of by instantiation of the smart device class
    SmartDevice::Request::registerRequestType("testRequest", typeid(TestRequest));

    // Test code for serialization and deserialization of configs
    // testConfigSerializationAndDeserialization();

    // Test code for deserialization of requests
    // testRequestDeserialization();

    // Test code for serialization of responses
    // testResponseSerialization();

    // Test code for the smart device event handling
    // testEventHandleing();

    ///////////////////////////////////////////////////////////////////////////
    // Test code for initiation
    ///////////////////////////////////////////////////////////////////////////

    ESP_LOGI(TAG, "\n\ntest code for initiation\n\n");

    // Create a Test Config
    TestDeviceConfig testConfig2(
        42,
        3.14,
        true,
        "Hello, world!",
        {1, 2, 3, 4, 5},
        make_unique<SmartDevice::BleConfig>("Test Device"),
        make_unique<SmartDevice::CloudConnectionConfig>(
            "deviceId",
            "jwt",
            "IP-in-the-hot-tub",
            "everytime",
            "mqttConnectionString"));

    // Create a TestDevice object
    TestDevice testDevice(std::move(testConfig2));

    // Initiate the device
    testDevice.initialize();

    ///////////////////////////////////////////////////////////////////////////
    // Test code for starting the device
    ///////////////////////////////////////////////////////////////////////////

    // Start the device
    testDevice.start();

    // Create an InitiateCloudConnection request
    string initiateCloudConnectionRequest = R"({
        "type": "InitiateCloudConnection",
        "data": {
            "deviceId": "deviceId",
            "jwt": "jwt",
            "ssid": "IP-in-the-hot-tub",
            "password": "everytime",
            "mqttConnectionString": "mqtt://10.11.2.80:188"
        }
    })";

    // Handle the InitiateCloudConnection request
    auto result = testDevice.handleRequest(initiateCloudConnectionRequest);
    if (!result.isSuccess())
    {
        ESP_LOGE(TAG, "failed to handle request: %s", result.getError().c_str());
    }

    if (!result.getValue().isSuccess())
    {
        ESP_LOGI(TAG, "failed to handle request with error for consumer: %s", result.getValue().getError().c_str());
    }

    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}