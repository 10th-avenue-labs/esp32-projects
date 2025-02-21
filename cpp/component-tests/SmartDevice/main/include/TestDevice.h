#pragma once

#include "SmartDevice.h"
#include "TestDeviceConfig.h"
#include "TestMessage.h"
#include "TestResponse.h"

const char *TEST_DEVICE_TAG = "TEST_DEVICE";

/**
 * @brief Test class for a smart device
 *
 */
class TestDevice : public SmartDevice::SmartDevice
{
public:
    TestDevice(TestDeviceConfig config) : SmartDevice::SmartDevice(std::move(config))
    {
        // Register event handlers
        registerMessageHandler<TestMessage>("testMessage", std::bind(&TestDevice::handleTestMessage, this, std::placeholders::_1));
    };

    Result<shared_ptr<ISerializable>> handleTestMessage(unique_ptr<IDeserializable> message)
    {
        ESP_LOGI(TEST_DEVICE_TAG, "handling test message 1");

        // Check if message is valid (not null)
        if (!message)
        {
            ESP_LOGE(TEST_DEVICE_TAG, "message is null");
            return Result<std::shared_ptr<ISerializable>>::createFailure("Message is null");
        }

        // Cast the message to the correct type
        auto testMessage = dynamic_cast<TestMessage *>(message.get());
        if (!testMessage)
        {
            return Result<shared_ptr<ISerializable>>::createFailure("Failed to cast message to TestMessage");
        }

        // Log the message contents
        ESP_LOGI(TEST_DEVICE_TAG, "myInt: %d", testMessage->myInt);
        ESP_LOGI(TEST_DEVICE_TAG, "myFloat: %f", testMessage->myFloat);
        ESP_LOGI(TEST_DEVICE_TAG, "myBool: %s", testMessage->myBool ? "true" : "false");
        ESP_LOGI(TEST_DEVICE_TAG, "myString: %s", testMessage->myString.c_str());
        ESP_LOGI(TEST_DEVICE_TAG, "myVector:");
        for (int i = 0; i < testMessage->myVector.size(); i++)
        {
            ESP_LOGI(TEST_DEVICE_TAG, "  %d", testMessage->myVector[i]);
        }

        // Create a response message
        auto response = std::make_shared<TestResponse>(42, 3.14, true, "Hello, world!", vector<int>{1, 2, 3, 4, 5});

        return Result<shared_ptr<ISerializable>>::createSuccess(response);
    }
};
