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
    /**
     * @brief The connection state of the smart device
     *
     */
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
        /**
         * @brief Construct a new Smart Device object
         *
         * @param cfg The configuration to use for the smart device
         * @param deviceType The type to register the smart device as
         * @param configUpdatedDelegate An optional config updated delegate to call when the device configuration is updated
         * @param maxWaitMs The maximum wait time for the smart device to wait for a connection with a default of 30 minutes
         */
        SmartDevice(SmartDeviceConfig cfg, string deviceType, std::function<void(void)> configUpdatedDelegate = nullptr, int maxWaitMs = 1000 * 60 * 30)
            : config(std::move(cfg)),
              deviceType(deviceType),
              configUpdatedDelegate(configUpdatedDelegate),
              waiter(WaitFunctions::ExponentialTime(2, 1000))
        {
            // Set the max wait time
            waiter.setMaxWaitMs(maxWaitMs);

            // Set the wifi disconnect handler
            WifiService::WifiService::onDisconnect = [this]
            { cloudDisconnectHandler(); };

            // Register event handlers
            registerRequestHandler<void>("GetDeviceInfo", std::bind(&SmartDevice::getDeviceInfoHandler, this, std::placeholders::_1));
            registerRequestHandler<BleConfig>("SetBleConfig", std::bind(&SmartDevice::setBleConfigHandler, this, std::placeholders::_1));
            registerRequestHandler<CloudConnectionConfig>("InitiateCloudConnection", std::bind(&SmartDevice::initiateCloudConnectionHandler, this, std::placeholders::_1));
        };

        /**
         * @brief Destroy the Smart Device object
         *
         */
        virtual ~SmartDevice() = default;

        /**
         * @brief Set the Config Updated Delegate
         *
         * @param delegate The delegate to call when the config is updated
         */
        void setConfigUpdatedDelegate(std::function<void(void)> delegate)
        {
            configUpdatedDelegate = delegate;
        }

        /**
         * @brief Register a request handler
         *
         * @tparam T The type of the argument for the request handler
         * @param requestType The request type
         * @param handler The request handler
         */
        template <typename T>
        void registerRequestHandler(string requestType, function<Result<shared_ptr<ISerializable>>(unique_ptr<IDeserializable>)> handler)
        {
            // Register the request type
            Request::registerRequestType(requestType, typeid(T));

            // Register the request handler
            requestHandlers[requestType] = handler;
        }

        /**
         * @brief Initialize the smart device
         *
         */
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

        /**
         * @brief Start the smart device
         *
         */
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
            if (!config.cloudConnectionConfig)
            {
                return;
            }

            // Initiate the cloud connection loop
            initiateCloudConnectionLoop();

            // Connect to the cloud
            Result result = connectCloud();
            if (!result.isSuccess())
            {
                ESP_LOGW(SMART_DEVICE_TAG, "failed to connect to cloud on startup: %s", result.getError().c_str());
            }
        }

        // private:
        SmartDeviceConfig config;
        std::string deviceType;
        Waiter waiter;
        std::unique_ptr<Mqtt::MqttClient> mqttClient;
        unique_ptr<AdtService> adtService;
        std::atomic<ConnectionState> connectionState = ConnectionState::NOT_CONNECTED;
        std::mutex stateMutex;
        std::condition_variable stateChanged;
        unordered_map<string, function<Result<shared_ptr<ISerializable>>(unique_ptr<IDeserializable>)>> requestHandlers;
        bool autoReconnect = false;
        std::function<void(void)> configUpdatedDelegate;

        ///////////////////////////////////////////////////////////////////////////
        // Message handling
        ///////////////////////////////////////////////////////////////////////////

        /**
         * @brief Handle messages from the arbitrary data transfer service. In practice, this processes the message and sends a response
         *
         * @param messageId The message id
         * @param message The message
         * @param device The device that sent the message
         */
        void adtMessageHandler(uint16_t messageId, vector<byte> message, shared_ptr<BleDevice> device)
        {
            // Convert the data to a string
            string messageString(
                reinterpret_cast<const char *>(message.data()),
                reinterpret_cast<const char *>(message.data()) + message.size());

            // Handle the message
            Result<Result<std::shared_ptr<ISerializable>>> result = handleRequest(messageString);
            if (!result.isSuccess())
            {
                ESP_LOGE(SMART_DEVICE_TAG, "failed to handle message: %s", result.getError().c_str());

                // Create a response
                Result errorResult = Result<std::shared_ptr<ISerializable>>::createFailure(format("Internal error occurred while attempting to handle request: %s", result.getError().c_str()));
                Response response("Response", make_unique<ResultResponderResponse>(messageId, errorResult));

                // Serialize the response request
                string serializedResponse = response.serializeToString();
                ESP_LOGI(SMART_DEVICE_TAG, "serialized response: %s", serializedResponse.c_str());

                // Convert the request to a vector of bytes
                vector<byte> responseBytes(
                    reinterpret_cast<const std::byte *>(serializedResponse.data()),
                    reinterpret_cast<const std::byte *>(serializedResponse.data()) + serializedResponse.size());

                // Send the response request
                adtService->sendMessage({device}, responseBytes);
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

        /**
         * @brief Handle messages from the mqtt client
         *
         * @param client The mqtt client
         * @param messageId The message id
         * @param data The message data
         */
        void mqttMessageHandler(Mqtt::MqttClient *client, int messageId, std::vector<std::byte> data)
        {
            // TODO
            ESP_LOGI(SMART_DEVICE_TAG, "mqtt message handler called");
            ESP_LOGI(SMART_DEVICE_TAG, "message id: %d", messageId);
            ESP_LOGI(SMART_DEVICE_TAG, "data length: %d", data.size());

            // Convert the data to a string
            string message(
                reinterpret_cast<const char *>(data.data()),
                reinterpret_cast<const char *>(data.data()) + data.size());
            ESP_LOGI(SMART_DEVICE_TAG, "message: %s", message.c_str());
        }

        /**
         * @brief Handle requests
         *
         * @param requestString The request string
         * @return Result<Result<std::shared_ptr<ISerializable>>> A result containing the result of the request handler.
         *   The outter result signifies an error with the meta request handling (ie deserialization of the request) - These errors are logged on the periferal device (this device).
         *   The inner result signifies an error with the request handler itself (ie failed to update ble name) - These errors are passed to the central.
         */
        Result<Result<std::shared_ptr<ISerializable>>> handleRequest(string requestString)
        {
            ESP_LOGI(SMART_DEVICE_TAG, "handling request: %s", requestString.c_str());

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

        ///////////////////////////////////////////////////////////////////////////
        // Request handlers
        ///////////////////////////////////////////////////////////////////////////

        /**
         * @brief Handle the GetDeviceInfo request
         *
         * @param request The request
         * @return Result<shared_ptr<ISerializable>> A result containing the device info
         */
        Result<shared_ptr<ISerializable>> getDeviceInfoHandler(unique_ptr<IDeserializable> request)
        {
            ESP_LOGI(SMART_DEVICE_TAG, "handling GetDeviceInfo request");

            // Create the device info
            DeviceInfo deviceInfo(deviceType, config.bleConfig->deviceName);

            // Return the device info request
            return Result<shared_ptr<ISerializable>>::createSuccess(make_shared<DeviceInfo>(deviceInfo));
        };

        /**
         * @brief Handle the SetBleConfig request
         *
         * @param request The request
         * @return Result<shared_ptr<ISerializable>> A result indicating success or failure
         */
        Result<shared_ptr<ISerializable>> setBleConfigHandler(unique_ptr<IDeserializable> request)
        {
            ESP_LOGI(SMART_DEVICE_TAG, "handling SetBleConfig request");

            // Cast the request to the correct type
            auto setBleConfigRequest = dynamic_cast<BleConfig *>(request.get());
            if (!setBleConfigRequest)
            {
                return Result<shared_ptr<ISerializable>>::createFailure("Failed to cast request to BleConfig");
            }

            // Update the BLE device name
            if (BleAdvertiser::setName(setBleConfigRequest->deviceName))
            {
                ESP_LOGI(SMART_DEVICE_TAG, "ble device name updated to '%s'", setBleConfigRequest->deviceName.c_str());
            }
            else
            {
                return Result<shared_ptr<ISerializable>>::createFailure("Failed to update ble device name");
            }

            // Update the configuration
            config.bleConfig->deviceName = setBleConfigRequest->deviceName;

            // Call the config updated delegate
            if (configUpdatedDelegate)
            {
                configUpdatedDelegate();
            }

            return Result<shared_ptr<ISerializable>>::createSuccess(nullptr);
        };

        /**
         * @brief Handle the InitiateCloudConnection request
         *
         * @param request The request
         * @return Result<shared_ptr<ISerializable>> A result indicating success or failure
         */
        Result<shared_ptr<ISerializable>> initiateCloudConnectionHandler(unique_ptr<IDeserializable> request)
        {
            // BUG: This function is not completely thread safe. If two threads call this function at the same time, 2 swaps of the cloud connection config will occur, which could lead to null pointer references

            // Cast the request to the correct type
            std::unique_ptr<CloudConnectionConfig> cloudConnectionConfig = std::unique_ptr<CloudConnectionConfig>(dynamic_cast<CloudConnectionConfig *>(request.release()));
            if (!cloudConnectionConfig)
            {
                return Result<shared_ptr<ISerializable>>::createFailure("Failed to cast request to CloudConnectionConfig");
            }

            // Swap the current clound config with the requested config
            config.cloudConnectionConfig.swap(cloudConnectionConfig);

            // Initiate the connection loop (this will enable reconnecting)
            initiateCloudConnectionLoop();

            // Attempt to connect to the cloud
            Result firstConnectionAttemptResult = connectCloud(); // With the connection loop initiated, this will trigger retries in a seperate task

            // Wait for the connection state to reach connected
            bool stateReached = waitConnectionState({ConnectionState::CONNECTED}, 60 * 1000);
            if (stateReached)
            {
                // Call the config updated delegate
                if (configUpdatedDelegate)
                {
                    configUpdatedDelegate();
                }

                return Result<shared_ptr<ISerializable>>::createSuccess(nullptr);
            }

            // Terminate the connection loop
            terminateConnectionLoop();

            // Wait for the connection state to reach settle
            bool terminalStateReached = waitConnectionState({ConnectionState::NOT_CONNECTED, ConnectionState::CONNECTED}, 10 * 1000);
            if (!terminalStateReached)
            {
                ESP_LOGW(SMART_DEVICE_TAG, "failed to reach a settled connection state after connection attempt");
            }

            // Disconnect from cloud
            Result result = disconnectCloud();
            if (!result.isSuccess())
            {
                ESP_LOGW(SMART_DEVICE_TAG, "failed to disconnect from cloud after connection attempt: %s", result.getError().c_str());
            }

            // Swap the config back
            config.cloudConnectionConfig.swap(cloudConnectionConfig);

            return Result<shared_ptr<ISerializable>>::createFailure(format("Failed to connect to cloud: {}", firstConnectionAttemptResult.getError().c_str()));
        }

        ///////////////////////////////////////////////////////////////////////////
        // Connection state handling (TODO: Abstract this to a library?)
        ///////////////////////////////////////////////////////////////////////////

        /**
         * @brief Set the Connection State
         *
         * @param newState The new connection state
         */
        void setConnectionState(ConnectionState newState)
        {
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                connectionState.store(newState, std::memory_order_relaxed);
            }

            // Notify any waiting threads
            stateChanged.notify_all();
        }

        /**
         * @brief Compare the current connection state with the expected state. If the values are equal, sets the connection state to the new state and returns true.
         * If the values are not equal, returns false and loads the actual value into the expectedState parameter.
         *
         * @param expectedState The expected state
         * @param newState The new state to set if the values are equal
         * @return true If the values are equal and the state was set to the new state
         * @return false If the values are not equal and the actual state was loaded into the expectedState parameter
         */
        bool setConnectionStateCompareExchange(ConnectionState &expectedState, ConnectionState newState)
        {
            bool changed = connectionState.compare_exchange_strong(expectedState, newState);
            if (changed)
            {
                // Notify any waiting threads
                stateChanged.notify_all();
            }

            return changed;
        }

        /**
         * @brief Get the Connection State
         *
         * @return ConnectionState The current connection state
         */
        ConnectionState getConnectionState()
        {
            return connectionState.load(std::memory_order_relaxed);
        }

        /**
         * @brief Wait for the connection state to reach one of the desired states
         *
         * @param connectionStates The desired connection states
         * @param timeoutMs A maximum duration of time to wait for the connection state to reach one of the desired states. A value of -1 indicates an indefinite wait.
         * @return true if the connection state reached one of the desired states
         * @return false if the connection state did not reach one of the desired states within the timeout
         */
        bool waitConnectionState(const std::vector<ConnectionState> &connectionStates, int timeoutMs = -1)
        {
            // TODO: Implement a way to cancel the wait

            std::unique_lock<std::mutex> lock(stateMutex);

            // Define a lambda to check if current state is in connectionStates
            auto isDesiredState = [&]()
            {
                return std::find(connectionStates.begin(), connectionStates.end(), getConnectionState()) != connectionStates.end();
            };

            if (timeoutMs < 0)
            {
                // Wait indefinitely until the state matches one of the desired states
                stateChanged.wait(lock, isDesiredState);
                return true;
            }
            else
            {
                // Wait with timeout
                return stateChanged.wait_for(lock, std::chrono::milliseconds(timeoutMs), isDesiredState);
            }
        }

        ///////////////////////////////////////////////////////////////////////////
        // Cloud connection logic
        ///////////////////////////////////////////////////////////////////////////

        /**
         * @brief Initiate the cloud connection loop. Note: This will not attempt to connect to the cloud
         *
         */
        void initiateCloudConnectionLoop()
        {
            // Reset the waiter
            waiter.reset();

            // Set the auto-reconnect flag
            autoReconnect = true;
        }

        /**
         * @brief Terminate the cloud connection loop. Note: This will not disconnect from the cloud
         *
         */
        void terminateConnectionLoop()
        {
            // Set the auto-reconnect flag
            autoReconnect = false;
        }

        /**
         * @brief Attempt to connect to the cloud
         *
         * @return Result<> A result indicating success or failure
         */
        Result<> connectCloud()
        {
            // TODO: Timeouts should be abstracted out and specified by the caller
            ESP_LOGI(SMART_DEVICE_TAG, "initiating cloud connection");

            // Atomically ensure that we're in the correct connection state (NOT_CONNECTED) and update the connection state to CONNECTING if so
            ConnectionState expectedValue = ConnectionState::NOT_CONNECTED;
            if (!setConnectionStateCompareExchange(expectedValue, ConnectionState::CONNECTING))
            {
                return Result<>::createFailure("Failed to connect to cloud, device is already in a connection state of: " + std::to_string(expectedValue));
            }

            // Verify that the cloud connection config is present
            if (!config.cloudConnectionConfig)
            {
                setConnectionState(ConnectionState::NOT_CONNECTED);
                return Result<>::createFailure("Cloud connection config is not present");
            }

            // Wait an appropriate amount of time
            Result waitResult = waiter.wait();
            if (!waitResult.isSuccess())
            {
                setConnectionState(ConnectionState::NOT_CONNECTED);
                return Result<>::createFailure(format("Failed to wait for an appropriate amount of time: {}", waitResult.getError().c_str()));
            }

            // Wait for the wifi connection state to settle such that it is not connecting or disconnecting
            if (!WifiService::WifiService::waitConnectionState({WifiService::ConnectionState::CONNECTED, WifiService::ConnectionState::NOT_CONNECTED}, 10 * 1000)) // 10 seconds as any operations are already pending
            {
                setConnectionState(ConnectionState::NOT_CONNECTED);
                return Result<>::createFailure("Timed out while waiting for wifi state to settle from pending actions");
            }

            // Connect to wifi if not connected already
            if (WifiService::WifiService::getConnectionState() != WifiService::ConnectionState::CONNECTED)
            {
                ESP_LOGI(SMART_DEVICE_TAG, "connecting to wifi");

                // Start connecting to wifi
                bool connectionStarted = WifiService::WifiService::startConnect({config.cloudConnectionConfig->ssid,
                                                                                 config.cloudConnectionConfig->password});
                if (!connectionStarted)
                {
                    setConnectionState(ConnectionState::NOT_CONNECTED);
                    return Result<>::createFailure("Failed to start connecting to wifi");
                }

                // Wait until we're either connected or disconnected
                if (!WifiService::WifiService::waitConnectionState({WifiService::ConnectionState::CONNECTED, WifiService::ConnectionState::NOT_CONNECTED}))
                {
                    setConnectionState(ConnectionState::NOT_CONNECTED);
                    return Result<>::createFailure("Timed out while waiting for wifi to reach desired state after attempting to connect");
                }

                // Check if we're connected
                if (WifiService::WifiService::getConnectionState() != WifiService::ConnectionState::CONNECTED)
                {
                    setConnectionState(ConnectionState::NOT_CONNECTED);
                    return Result<>::createFailure("Failed to connect to wifi");
                }
            }

            // Create the mqtt client if not done so already
            bool firstConnection = !mqttClient;
            if (firstConnection)
            {
                ESP_LOGI(SMART_DEVICE_TAG, "creating mqtt client");
                mqttClient = make_unique<Mqtt::MqttClient>(config.cloudConnectionConfig->mqttConnectionString);
                mqttClient->onDisconnected = [this](Mqtt::MqttClient *)
                { cloudDisconnectHandler(); };
            }
            else
            {
                ESP_LOGI(SMART_DEVICE_TAG, "mqtt client already exists, updating connection string");

                // Update the mqtt client connection string
                mqttClient->setBrokerUri(config.cloudConnectionConfig->mqttConnectionString);
            }

            // Wait for the mqtt connection state to settle such that it is not connecting or disconnecting
            if (!mqttClient->waitConnectionState({Mqtt::ConnectionState::CONNECTED, Mqtt::ConnectionState::NOT_CONNECTED}, 10 * 1000)) // 10 seconds as any operations are already pending
            {
                setConnectionState(ConnectionState::NOT_CONNECTED);
                return Result<>::createFailure("Timed out while waiting for mqtt state to settle from pending actions");
            }

            // Connect to mqtt if not connected already
            if (mqttClient->getConnectionState() != Mqtt::ConnectionState::CONNECTED)
            {
                ESP_LOGI(SMART_DEVICE_TAG, "connecting to mqtt broker");

                // Start connecting to the mqtt broker
                if (firstConnection)
                {
                    mqttClient->connect();
                }
                else
                {
                    mqttClient->reconnect();
                }

                // Wait until we're either connected or disconnected
                if (!mqttClient->waitConnectionState({Mqtt::ConnectionState::CONNECTED, Mqtt::ConnectionState::NOT_CONNECTED}))
                {
                    setConnectionState(ConnectionState::NOT_CONNECTED);
                    return Result<>::createFailure("Timed out while waiting for mqtt to reach desired state after attempting to connect");
                }

                // Check if we're connected
                if (mqttClient->getConnectionState() != Mqtt::ConnectionState::CONNECTED)
                {
                    setConnectionState(ConnectionState::NOT_CONNECTED);
                    return Result<>::createFailure("Failed to connect to mqtt broker");
                }
            }

            // Subscribe to the topic if not done so already
            mqttClient->subscribe(
                format("device/{}/requested_state", config.cloudConnectionConfig->deviceId),
                std::bind(&SmartDevice::mqttMessageHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            // Reset the waiter
            waiter.reset();

            ESP_LOGI(SMART_DEVICE_TAG, "successfully connected to cloud");
            setConnectionState(ConnectionState::CONNECTED);
            return Result<>::createSuccess();
        }

        /**
         * @brief Disconnect from the cloud
         *
         * @return Result<> A result indicating success or failure
         */
        Result<> disconnectCloud()
        {
            // Check if we're already disconnected
            // FIXME: We are partially connected in some scenarios, ie wifi connected but not mqtt connected. Should still disconnect these items
            if (getConnectionState() == ConnectionState::NOT_CONNECTED)
            {
                ESP_LOGI(SMART_DEVICE_TAG, "already disconnected");
                return Result<>::createSuccess();
            }

            // Atomically check that we're in a connected state and update the state to DISCONNECTING if so
            ConnectionState expectedValue = ConnectionState::CONNECTED;
            if (!setConnectionStateCompareExchange(expectedValue, ConnectionState::DISCONNECTING))
            {
                return Result<>::createFailure(format("Failed to disconnect from cloud, device is not in a connected state, current state: %d", (int)expectedValue));
            }

            // Destroy the mqtt client
            if (mqttClient)
            {
                mqttClient.reset();
            }

            // Disconnect from wifi
            WifiService::WifiService::startDisconnect();

            // Wait for the wifi connection state to settle
            bool desiredStateReached = WifiService::WifiService::waitConnectionState({WifiService::ConnectionState::NOT_CONNECTED}, 10000);

            // Set the connection state to not connected
            setConnectionState(ConnectionState::NOT_CONNECTED);

            // Return the result
            return desiredStateReached ? Result<>::createSuccess() : Result<>::createFailure("Failed to disconnect from cloud, disconnecting from wifi timed out");
        }

        /**
         * @brief Handle disconnects from the cloud. Will trigger a reconnection attempt if the auto-reconnect flag is set and we're not in a DISCONNECTING or NOT_CONNECTED state
         *
         */
        void cloudDisconnectHandler()
        {
            // Atomically check that we're in a connected state and update the state to NOT_CONNECTED if so
            ConnectionState expectedValue = ConnectionState::CONNECTED;
            bool deviceWasConnected = setConnectionStateCompareExchange(expectedValue, ConnectionState::NOT_CONNECTED);

            ESP_LOGI(SMART_DEVICE_TAG, "disconnected from cloud, handling disconnect from a connection state of %d", expectedValue);

            // Check if the connection state was DISCONNECTING
            if (expectedValue == ConnectionState::DISCONNECTING)
            {
                // Do not try to reconnect
                ESP_LOGI(SMART_DEVICE_TAG, "not attempting to reconnect to cloud from a state of %d", expectedValue);
                return;
            }

            // Check if the auto-reconnect flag is set
            if (!autoReconnect)
            {
                // Do not try to reconnect
                ESP_LOGI(SMART_DEVICE_TAG, "auto-reconnect flag is not set, not attempting to reconnect");
                return;
            }

            // Check if the connection state was CONNECTING
            if (expectedValue == ConnectionState::CONNECTING)
            {
                // Wait for the connection state to settle
                ESP_LOGI(SMART_DEVICE_TAG, "waiting for connection state to settle before attempting reconnect, current state %d", getConnectionState());
                bool desiredStateReached = waitConnectionState({ConnectionState::NOT_CONNECTED}, 10000);
                if (!desiredStateReached)
                {
                    ESP_LOGE(SMART_DEVICE_TAG, "failed to reach desired state handling disconnect from cloud");
                    return;
                }
            }

            // Call the connect cloud function
            Result result = connectCloud();
            if (!result.isSuccess())
            {
                ESP_LOGW(SMART_DEVICE_TAG, "failed to reconnect to cloud: %s", result.getError().c_str());
            }
        }
    };
};
