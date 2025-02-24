#pragma once

#include "Result.h"
#include "MqttClient.h"
#include "config/SmartDeviceConfig.h"
#include "AdtService.h"
#include "IDeserializable.h"
#include "ISerializable.h"
#include "Message.h"
#include "BleAdvertiser.h"
#include "WifiService.h"
#include "Waiter.h"

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
        SmartDevice(SmartDeviceConfig cfg)
            : config(std::move(cfg)),
              waiter(WaitFunctions::ExponentialTime(2, 1000))
        {
            waiter.setMaxWaitMs(1000 * 60 * 30); // 30 minutes
        };

        virtual ~SmartDevice() = default;

        template <typename T>
        void registerMessageHandler(string messageType, function<Result<shared_ptr<ISerializable>>(unique_ptr<IDeserializable>)> handler)
        {
            // Register the message type
            Message::registerMessageType(messageType, typeid(T));

            // Register the message handler
            messageHandlers[messageType] = handler;
        }

        Result<std::shared_ptr<ISerializable>> handleMessage(string messageString)
        {
            // Get the message deserializer
            auto messageDeserializer = IDeserializable::getDeserializer<Message>();
            if (!messageDeserializer)
            {
                return Result<std::shared_ptr<ISerializable>>::createFailure("Failed to get message deserializer");
            }

            // Deserialize the message
            auto deserializedMessageResult = messageDeserializer(cJSON_Parse(messageString.c_str()));
            if (!deserializedMessageResult.isSuccess())
            {
                return Result<std::shared_ptr<ISerializable>>::createFailure("Failed to deserialize message: " + deserializedMessageResult.getError());
            }

            // Transfer ownership from the result and cast the message
            std::unique_ptr<Message> deserializedMessage = std::unique_ptr<Message>(dynamic_cast<Message *>(deserializedMessageResult.getValue().release()));
            ESP_LOGI(SMART_DEVICE_TAG, "message type: %s", deserializedMessage->type.c_str());

            // Get the message handler
            auto it = messageHandlers.find(deserializedMessage->type);
            if (it == messageHandlers.end())
            {
                return Result<std::shared_ptr<ISerializable>>::createFailure("No handler found for message type '" + deserializedMessage->type + "'");
            }
            auto messageHandler = it->second;

            // Get the message data
            std::unique_ptr<IDeserializable> messagePayload = std::unique_ptr<IDeserializable>(deserializedMessage->data.release());

            // Call the message handler
            return messageHandler(std::move(messagePayload));
        }

        // void adtMessageHandler(uint16_t messageId, vector<byte> message, shared_ptr<BleDevice> device)
        // {
        //     // Convert the data to a string
        //     string messageString(
        //         reinterpret_cast<const char *>(message.data()),
        //         reinterpret_cast<const char *>(message.data()) + message.size());

        //     // TODO: Handle the message
        // }

        void initialize()
        {
            // // Create the ADT service
            // ESP_LOGI(SMART_DEVICE_TAG, "creating adt service");
            // adtService = make_unique<AdtService>(
            //     ADT_SERVICE_UUID,
            //     ADT_SERVICE_MTU_CHARACTERISTIC_UUID,
            //     ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID,
            //     ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID,
            //     adtMessageHandler);

            // // Initialize the BLE advertiser
            // ESP_LOGI(SMART_DEVICE_TAG, "initializing ble advertiser");
            // BleAdvertiser::init(
            //     config.bleConfig->deviceName,
            //     BLE_GAP_APPEARANCE_GENERIC_TAG,
            //     BLE_GAP_LE_ROLE_PERIPHERAL,
            //     {adtService->getBleService()},
            //     nullptr // No device connected handler needed
            // );

            // // Initialize the wifi service
            // ESP_LOGI(SMART_DEVICE_TAG, "initializing wifi service");
            // WifiService::WifiService::init();
        }

        void start()
        {
            // Start the BLE advertiser
            ESP_LOGI(SMART_DEVICE_TAG, "starting ble advertiser");
            BleAdvertiser::advertise();

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

    private:
        atomic<int> connectionState{ConnectionState::NOT_CONNECTED};
        SmartDeviceConfig config;
        Waiter waiter;
        unordered_map<string, function<Result<shared_ptr<ISerializable>>(unique_ptr<IDeserializable>)>> messageHandlers;
        unique_ptr<AdtService> adtService;
    };
};
