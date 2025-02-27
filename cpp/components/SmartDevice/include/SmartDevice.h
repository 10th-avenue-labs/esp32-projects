#pragma once

#include "Result.h"
#include "MqttClient.h"
#include "config/SmartDeviceConfig.h"
#include "AdtService.h"
#include "IDeserializable.h"
#include "ISerializable.h"
#include "Request.h"
#include "BleAdvertiser.h"
#include "WifiService.h"
#include "Waiter.h"
#include "responses/DeviceInfo.h"
#include "Response.h"
#include "ResultResponderResponse.h"

#include <unordered_map>
#include <functional>

// Define the ADT service and characteristic UUIDs
#define ADT_SERVICE_UUID "cc3aab60-0001-0000-81e0-c88f19fb28cb"
#define ADT_SERVICE_MTU_CHARACTERISTIC_UUID "cc3aab60-0001-0001-81e0-c88f19fb28cb"
#define ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID "cc3aab60-0001-0002-81e0-c88f19fb28cb"
#define ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID "cc3aab60-0001-0003-81e0-c88f19fb28cb"

// BLE constants
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

static const char *SMART_DEVICE_TAG = "SMART_DEVICE";

namespace SmartDevice
{
    enum ConnectionState
    {
        NOT_CONNECTED,
        CONNECTING,
        CONNECTED,
        DISCONNECTING
    };

    class SmartDevice
    {
    public:
        SmartDevice(SmartDeviceConfig cfg, string deviceType)
            : config(std::move(cfg)),
              deviceType(deviceType),
              waiter(WaitFunctions::ExponentialTime(2, 1000))
        {
            // Set the max wait time to 30 minutes
            waiter.setMaxWaitMs(1000 * 60 * 30); // 30 minutes

            // Register event handlers
            registerRequestHandler<void>("GetDeviceInfo", std::bind(&SmartDevice::getDeviceInfo, this, std::placeholders::_1));
        };

        virtual ~SmartDevice() = default;

        template <typename T>
        void registerRequestHandler(string requestType, function<Result<shared_ptr<ISerializable>>(unique_ptr<IDeserializable>)> handler)
        {
            // Register the request type
            Request::registerRequestType(requestType, typeid(T));

            // Register the request handler
            requestHandlers[requestType] = handler;
        }

        Result<Result<std::shared_ptr<ISerializable>>> handleRequest(string requestString)
        {
            // Get the request deserializer
            auto requestDeserializer = IDeserializable::getDeserializer<Request>();
            if (!requestDeserializer)
            {
                return Result<Result<std::shared_ptr<ISerializable>>>::createFailure("Failed to get request deserializer");
            }

            // Deserialize the request
            auto deserializedRequestResult = requestDeserializer(cJSON_Parse(requestString.c_str()));
            if (!deserializedRequestResult.isSuccess())
            {
                return Result<Result<std::shared_ptr<ISerializable>>>::createFailure("Failed to deserialize request: " + deserializedRequestResult.getError());
            }

            // Transfer ownership from the result and cast the request
            std::unique_ptr<Request> deserializedRequest = std::unique_ptr<Request>(dynamic_cast<Request *>(deserializedRequestResult.getValue().release()));
            ESP_LOGI(SMART_DEVICE_TAG, "request type: %s", deserializedRequest->type.c_str());

            // Get the request handler
            auto it = requestHandlers.find(deserializedRequest->type);
            if (it == requestHandlers.end())
            {
                ESP_LOGI(SMART_DEVICE_TAG, "request handlers registered for types:");
                for (auto &handler : requestHandlers)
                {
                    ESP_LOGI(SMART_DEVICE_TAG, "  %s", handler.first.c_str());
                }
                return Result<Result<std::shared_ptr<ISerializable>>>::createFailure("No handler found for request type '" + deserializedRequest->type + "'");
            }
            auto requestHandler = it->second;

            // Get the request data
            std::unique_ptr<IDeserializable> requestPayload = std::unique_ptr<IDeserializable>(deserializedRequest->data.release());

            // Call the request handler
            return Result<Result<std::shared_ptr<ISerializable>>>::createSuccess(requestHandler(std::move(requestPayload)));
        }

        void adtMessageHandler(uint16_t messageId, vector<byte> request, shared_ptr<BleDevice> device)
        {
            // Convert the data to a string
            string requestString(
                reinterpret_cast<const char *>(request.data()),
                reinterpret_cast<const char *>(request.data()) + request.size());

            // Handle the request
            Result<Result<std::shared_ptr<ISerializable>>> result = handleRequest(requestString);
            if (!result.isSuccess())
            {
                ESP_LOGE(SMART_DEVICE_TAG, "failed to handle request: %s", result.getError().c_str());
                // TODO: Send an error response
                return;
            }

            // Get the handler result
            Result<std::shared_ptr<ISerializable>> handlerResult = result.getValue();

            // Create a response
            Response response("Response", make_unique<ResultResponderResponse>(messageId, handlerResult));

            // Serialize the response request
            string serializedResponse = response.serializeToString();
            ESP_LOGI(SMART_DEVICE_TAG, "serialized response: %s", serializedResponse.c_str());

            // Convert the request to a vector of bytes
            vector<byte> responseBytes(
                reinterpret_cast<const std::byte *>(serializedResponse.data()),
                reinterpret_cast<const std::byte *>(serializedResponse.data()) + serializedResponse.size());

            // Send the response request
            adtService->sendMessage({device}, responseBytes);
        }

        void initialize()
        {
            // Create the ADT service
            ESP_LOGI(SMART_DEVICE_TAG, "creating adt service");
            adtService = make_unique<AdtService>(
                ADT_SERVICE_UUID,
                ADT_SERVICE_MTU_CHARACTERISTIC_UUID,
                ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID,
                ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID,
                [this](uint16_t messageId, std::vector<std::byte> data, std::shared_ptr<BleDevice> device)
                {
                    ESP_LOGI(SMART_DEVICE_TAG, "lambda request handlers length: %d", requestHandlers.size());
                    this->adtMessageHandler(messageId, std::move(data), std::move(device));
                });

            // Initialize the BLE advertiser
            ESP_LOGI(SMART_DEVICE_TAG, "initializing ble advertiser");
            BleAdvertiser::init(
                config.bleConfig->deviceName,
                BLE_GAP_APPEARANCE_GENERIC_TAG,
                BLE_GAP_LE_ROLE_PERIPHERAL,
                {adtService->getBleService()},
                nullptr // No device connected handler needed
            );

            // Initialize the wifi service
            ESP_LOGI(SMART_DEVICE_TAG, "initializing wifi service");
            WifiService::WifiService::init();
        }

        void start()
        {
            // FIXME: When we create a task here, that means that the function is non-blocking and will quickly return
            // When this returns, if our main function returns, then this smart device will go out of scope and request handlers
            // will be deallocated. Not sure why but that's what I'm observing when removing the while true loop in main.cpp

            // Start the BLE advertiser in a separate task
            ESP_LOGI(SMART_DEVICE_TAG, "starting ble advertiser");
            xTaskCreate(
                [](void *)
                {
                    BleAdvertiser::advertise();
                },
                "ble_advertiser",
                4096,
                nullptr,
                5,
                nullptr);

            // Check if we have a cloud configuration
            if (true)
            {
                // Initiate the cloud connection loop

                // Connect to the cloud
            }
        }

        void initiateCloudConnectionLoop()
        {
            // Reset connection info

            // Set the wifi disconnect handler

            // Set the mqtt disconnect handler
        }

        Result<void> connectCloud()
        {
            // Atomically ensure that we're in the correct connection state (NOT_CONNECTED) and update the connection state to CONNECTING if so
            int expectedValue = ConnectionState::NOT_CONNECTED;
            if (!connectionState.compare_exchange_strong(expectedValue, ConnectionState::CONNECTING))
            {
                return Result<>::createFailure(format("Failed to connect to cloud, device is already in a connection state of: %d", expectedValue));
            }

            // Wait an appropriate amount of time
            waiter.wait();

            // Connect to wifi if not connected already
            if (WifiService::WifiService::getConnectionState() != WifiService::ConnectionState::CONNECTED)
            {
                // Connect to wifi
                // auto wifiConfig = config.cloudConnectionConfig->getWifiConfig();
            }

            return Result<>::createFailure("Not implemented");
        }

        ///////////////////////////////////////////////////////////////////////////
        // Request handlers
        ///////////////////////////////////////////////////////////////////////////

        Result<shared_ptr<ISerializable>> getDeviceInfo(unique_ptr<IDeserializable> data)
        {
            ESP_LOGI(SMART_DEVICE_TAG, "handling GetDeviceInfo request");

            // Create the device info request
            DeviceInfo deviceInfo(deviceType, config.bleConfig->deviceName);

            // Return the device info request
            return Result<shared_ptr<ISerializable>>::createSuccess(make_shared<DeviceInfo>(deviceInfo));

            // return Result<shared_ptr<ISerializable>>::createFailure("Not implemented");
        };

    private:
        SmartDeviceConfig config;
        std::string deviceType;
        Waiter waiter;
        atomic<int> connectionState{ConnectionState::NOT_CONNECTED};
        unordered_map<string, function<Result<shared_ptr<ISerializable>>(unique_ptr<IDeserializable>)>> requestHandlers;
        unique_ptr<AdtService> adtService;
    };
};
