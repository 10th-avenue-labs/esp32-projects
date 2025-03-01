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

            // Set the wifi disconnect handler
            WifiService::WifiService::onDisconnect = [this]
            { cloudDisconnectHandler(); };

            // Register event handlers
            registerRequestHandler<void>("GetDeviceInfo", std::bind(&SmartDevice::getDeviceInfoHandler, this, std::placeholders::_1));
            registerRequestHandler<BleConfig>("SetBleConfig", std::bind(&SmartDevice::setBleConfigHandler, this, std::placeholders::_1));
            registerRequestHandler<CloudConnectionConfig>("InitiateCloudConnection", std::bind(&SmartDevice::initiateCloudConnectionHandler, this, std::placeholders::_1));
        };

        virtual ~SmartDevice() = default;

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
        int retries = 0;

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

            // TODO: Call the config updated delegate (This will likely save the config to NVS but is a task handled by the consumer of this class)

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
            ESP_LOGI(SMART_DEVICE_TAG, "INITIATE handling InitiateCloudConnection request");

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
            Result result = connectCloud();
            if (!result.isSuccess()) // Debug
            {
                ESP_LOGI(SMART_DEVICE_TAG, "INITIATE failed to connect to cloud: %s", result.getError().c_str());
            }

            // Wait for the connection state to reach connected
            ESP_LOGI(SMART_DEVICE_TAG, "INITIATE waiting for connection state to reach connected");
            bool stateReached = waitConnectionState({ConnectionState::CONNECTED}, 60 * 1000);
            ESP_LOGI(SMART_DEVICE_TAG, "INITIATE connection state reached: %d", stateReached);
            ESP_LOGI(SMART_DEVICE_TAG, "INITIATE connection state: %d", getConnectionState());
            if (stateReached)
            {
                // Call the config updated delegate
                // TODO: This will likely save the config to NVS but is a task handled by the consumer of this class

                return Result<shared_ptr<ISerializable>>::createSuccess(nullptr);
            }

            // Terminate the connection loop
            terminateConnectionLoop();

            // Disconnect from cloud
            // TODO

            // Swap the config back
            config.cloudConnectionConfig.swap(cloudConnectionConfig);

            return Result<shared_ptr<ISerializable>>::createFailure("Failed to connect to cloud");
        }

        ///////////////////////////////////////////////////////////////////////////
        // Connection state handling
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
            // Generate a unique id for the connection attempt
            string connectionAttemptId = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connecting to cloud", connectionAttemptId.c_str());

            // Atomically ensure that we're in the correct connection state (NOT_CONNECTED) and update the connection state to CONNECTING if so
            ConnectionState expectedValue = ConnectionState::NOT_CONNECTED;
            if (!setConnectionStateCompareExchange(expectedValue, ConnectionState::CONNECTING))
            {
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connect attempt failed", connectionAttemptId.c_str());
                return Result<>::createFailure(format("Failed to connect to cloud, device is already in a connection state of: %d", (int)expectedValue));
            }

            // Verify that the cloud connection config is present
            if (!config.cloudConnectionConfig)
            {
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connect attempt failed", connectionAttemptId.c_str());
                setConnectionState(ConnectionState::NOT_CONNECTED);
                return Result<>::createFailure("Cloud connection config is not present");
            }

            // Wait an appropriate amount of time
            Result res = waiter.wait();
            if (!res.isSuccess())
            {
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connect attempt failed - failed to wait for an appropriate amount of time", connectionAttemptId.c_str());
                setConnectionState(ConnectionState::NOT_CONNECTED);
                return Result<>::createFailure("Failed to wait for an appropriate amount of time");
            }

            // Wait for the wifi connection state to settle such that it is not connecting or disconnecting
            // TODO: Add timeout
            ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s waiting for wifi connection state to settle, current state: %d", connectionAttemptId.c_str(), WifiService::WifiService::getConnectionState());
            WifiService::WifiService::waitConnectionState({WifiService::ConnectionState::CONNECTED, WifiService::ConnectionState::NOT_CONNECTED});
            ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s wifi connection state settled, current state: %d", connectionAttemptId.c_str(), WifiService::WifiService::getConnectionState());

            // Connect to wifi if not connected already
            ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connecting to wifi, current wifi state %d", connectionAttemptId.c_str(), WifiService::WifiService::getConnectionState());
            if (WifiService::WifiService::getConnectionState() != WifiService::ConnectionState::CONNECTED)
            {
                // Start connecting to wifi
                bool connectionStarted = WifiService::WifiService::startConnect({config.cloudConnectionConfig->ssid,
                                                                                 config.cloudConnectionConfig->password});
                if (!connectionStarted)
                {
                    ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connect attempt failed - failed to start connecting to wifi", connectionAttemptId.c_str());
                    setConnectionState(ConnectionState::NOT_CONNECTED);
                    return Result<>::createFailure("Failed to start connecting to wifi");
                }

                // Wait until we're either connected or disconnected
                // TODO: Add and handle timeout
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s waiting for wifi connection state to settle, current state %d", connectionAttemptId.c_str(), WifiService::WifiService::getConnectionState());
                bool desiredStateReached = WifiService::WifiService::waitConnectionState({WifiService::ConnectionState::CONNECTED, WifiService::ConnectionState::NOT_CONNECTED});
                // bool desiredStateReached = WifiService::WifiService::waitConnectionState({WifiService::ConnectionState::CONNECTED, WifiService::ConnectionState::NOT_CONNECTED}, 10000);
                if (!desiredStateReached)
                {
                    ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connect attempt failed - failed to reach desired state after connecting to wifi, current state: %d", connectionAttemptId.c_str(), WifiService::WifiService::getConnectionState());
                    setConnectionState(ConnectionState::NOT_CONNECTED);
                    return Result<>::createFailure("Failed to reach desired state after connecting to wifi");
                }
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s wifi connection state settled, current state %d", connectionAttemptId.c_str(), WifiService::WifiService::getConnectionState());

                // Check if we're connected
                if (WifiService::WifiService::getConnectionState() != WifiService::ConnectionState::CONNECTED)
                {
                    ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connect attempt failed - failed to connect to wifi", connectionAttemptId.c_str());
                    setConnectionState(ConnectionState::NOT_CONNECTED);
                    return Result<>::createFailure("Failed to connect to wifi");
                }
            }

            // Create the mqtt client if not done so already
            bool firstConnection = !mqttClient;
            if (firstConnection)
            {
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s creating mqtt client", connectionAttemptId.c_str());
                mqttClient = make_unique<Mqtt::MqttClient>(config.cloudConnectionConfig->mqttConnectionString);
                mqttClient->onDisconnected = [this](Mqtt::MqttClient *)
                { cloudDisconnectHandler(); };
            }
            else
            {
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s mqtt client already exists, updating connection string", connectionAttemptId.c_str());
                // Update the mqtt client connection string
                mqttClient->setBrokerUri(config.cloudConnectionConfig->mqttConnectionString);
            }

            // Wait for the mqtt connection state to settle such that it is not connecting or disconnecting
            // TODO: Add timeout
            ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s waiting for mqtt connection state to settle, current state: %d", connectionAttemptId.c_str(), mqttClient->getConnectionState());
            mqttClient->waitConnectionState({Mqtt::ConnectionState::CONNECTED, Mqtt::ConnectionState::NOT_CONNECTED});
            ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s mqtt connection state settled, current state: %d", connectionAttemptId.c_str(), mqttClient->getConnectionState());

            // Connect to mqtt if not connected already
            if (mqttClient->getConnectionState() != Mqtt::ConnectionState::CONNECTED)
            {
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connecting to mqtt", connectionAttemptId.c_str());
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
                // TODO: Add and handle timeout
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s waiting for mqtt connection state to settle, current state: %d", connectionAttemptId.c_str(), mqttClient->getConnectionState());
                mqttClient->waitConnectionState({Mqtt::ConnectionState::CONNECTED, Mqtt::ConnectionState::NOT_CONNECTED});
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s mqtt connection state settled, current state: %d", connectionAttemptId.c_str(), mqttClient->getConnectionState());

                // Check if we're connected
                if (mqttClient->getConnectionState() != Mqtt::ConnectionState::CONNECTED)
                {
                    ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connect attempt failed", connectionAttemptId.c_str());
                    setConnectionState(ConnectionState::NOT_CONNECTED);
                    return Result<>::createFailure("Failed to connect to mqtt broker");
                }
            }

            // Subscribe to the topic if not done already
            // TODO

            // Verify that the connection state was not interrupted while connecting
            // FIXME: The following sections might not be entirely thread-safe. What if a disconnect happens after the following read?
            ConnectionState endingState = getConnectionState();
            if (endingState != ConnectionState::CONNECTING)
            {
                ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connect attempt failed", connectionAttemptId.c_str());
                setConnectionState(ConnectionState::NOT_CONNECTED);
                return Result<>::createFailure(format("Connection was interrupted while connecting. Ending state: %d", (int)endingState));
            }

            // Set the connection state to connected
            ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connected to cloud", connectionAttemptId.c_str());

            ESP_LOGI(SMART_DEVICE_TAG, "CONNECT %s connect attempt completed", connectionAttemptId.c_str());
            setConnectionState(ConnectionState::CONNECTED);
            return Result<>::createSuccess();
        }

        /**
         * @brief Handle disconnects from the cloud. Will trigger a reconnection attempt if the auto-reconnect flag is set
         *
         */
        void cloudDisconnectHandler()
        {
            // Generate a unique id for the connection attempt
            string disconnectId = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            retries++;

            ESP_LOGI(SMART_DEVICE_TAG, "DISCONNECT %s retry count %d", disconnectId.c_str(), retries);

            if (retries == 3)
            {
                config.cloudConnectionConfig->mqttConnectionString = "mqtt://test.mosquitto.org:1883";
            }

            // Get stack high-water mark (minimum free stack space in words)
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGI(SMART_DEVICE_TAG, "DISCONNECT %s Stack high-water mark: %d bytes", disconnectId.c_str(), stackLeft * sizeof(StackType_t));

            ESP_LOGI(SMART_DEVICE_TAG, "DISCONNECT %s disconnected from cloud, handling disconnect", disconnectId.c_str());
            ESP_LOGI(SMART_DEVICE_TAG, "DISCONNECT %s connection state: %d", disconnectId.c_str(), getConnectionState());

            ConnectionState currentState = getConnectionState();

            // Wait for the connection state to settle
            ESP_LOGI(SMART_DEVICE_TAG, "DISCONNECT %s waiting for connection state to settle, current state %d", disconnectId.c_str(), currentState);
            bool desiredStateReached = waitConnectionState({ConnectionState::NOT_CONNECTED}, 10000);
            if (!desiredStateReached)
            {
                ESP_LOGE(SMART_DEVICE_TAG, "DISCONNECT %s failed to reach desired state after disconnecting from cloud", disconnectId.c_str());
                return;
            }
            ESP_LOGI(SMART_DEVICE_TAG, "DISCONNECT %s connection state settled, current state %d", disconnectId.c_str(), getConnectionState());

            // Check if the auto-reconnect flag is set
            if (!autoReconnect)
            {
                // Do not try to reconnect
                ESP_LOGI(SMART_DEVICE_TAG, "DISCONNECT %s auto-reconnect flag is not set, not attempting to reconnect", disconnectId.c_str());
                return;
            }

            // Call the connect cloud function
            Result result = connectCloud();
            if (!result.isSuccess())
            {
                ESP_LOGW(SMART_DEVICE_TAG, "DISCONNECT %s failed to reconnect to cloud: %s", disconnectId.c_str(), result.getError().c_str());
            }
            ESP_LOGI(SMART_DEVICE_TAG, "DISCONNECT %s reconnect attempt complete", disconnectId.c_str());
        }
    };
};
