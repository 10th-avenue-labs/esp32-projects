#pragma once

#include "SmartDevice.h"
#include "TestDeviceConfig.h"
#include "TestRequest.h"
#include "TestResponse.h"

const char *TEST_DEVICE_TAG = "TEST_DEVICE";

/**
 * @brief Test class for a smart device
 *
 */
class TestDevice : public SmartDevice::SmartDevice
{
public:
    /**
     * @brief Construct a new Test Device object
     *
     * @param config The config to use for the TestDevice
     */
    TestDevice(TestDeviceConfig config) : SmartDevice::SmartDevice(std::move(config), "SmartPlug") // Re-using SmartPlug device type instead of TestDevice for testing with front-end
    {
        // Register event handlers
        registerRequestHandler<TestRequest>("testRequest", std::bind(&TestDevice::handleTestRequest, this, std::placeholders::_1));
    };

    /**
     * @brief Handle test requests
     *
     * @param request The deserializable test request
     * @return Result<shared_ptr<ISerializable>> A result containing the response
     */
    Result<shared_ptr<ISerializable>> handleTestRequest(unique_ptr<IDeserializable> request)
    {
        ESP_LOGI(TEST_DEVICE_TAG, "handling test request 1");

        // Check if request is valid (not null)
        if (!request)
        {
            ESP_LOGE(TEST_DEVICE_TAG, "request is null");
            return Result<std::shared_ptr<ISerializable>>::createFailure("Request is null");
        }

        // Cast the request to the correct type
        auto testRequest = dynamic_cast<TestRequest *>(request.get());
        if (!testRequest)
        {
            return Result<shared_ptr<ISerializable>>::createFailure("Failed to cast request to TestRequest");
        }

        // Log the request contents
        ESP_LOGI(TEST_DEVICE_TAG, "myInt: %d", testRequest->myInt);
        ESP_LOGI(TEST_DEVICE_TAG, "myFloat: %f", testRequest->myFloat);
        ESP_LOGI(TEST_DEVICE_TAG, "myBool: %s", testRequest->myBool ? "true" : "false");
        ESP_LOGI(TEST_DEVICE_TAG, "myString: %s", testRequest->myString.c_str());
        ESP_LOGI(TEST_DEVICE_TAG, "myVector:");
        for (int i = 0; i < testRequest->myVector.size(); i++)
        {
            ESP_LOGI(TEST_DEVICE_TAG, "  %d", testRequest->myVector[i]);
        }

        // Create a response request
        auto response = std::make_shared<TestResponse>(42, 3.14, true, "Hello, world!", vector<int>{1, 2, 3, 4, 5});

        return Result<shared_ptr<ISerializable>>::createSuccess(response);
    }
};
