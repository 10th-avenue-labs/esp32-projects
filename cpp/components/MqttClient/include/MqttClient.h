#pragma once

#include "Result.h"

#include <string>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <map>
#include <functional>
#include <mqtt_client.h>
#include <esp_log.h>

namespace Mqtt
{
    static const char *TAG = "MQTT_CLIENT";

    enum ConnectionState
    {
        NOT_CONNECTED,
        CONNECTING,
        CONNECTED,
        DISCONNECTING
    };

    class MqttClient
    {
    public:
        std::function<void(MqttClient *)> onConnected;    // Delegate for when the client is connected
        std::function<void(MqttClient *)> onDisconnected; // Delegate for when the client is disconnected

        ///////////////////////////////////////////////////////////////////////
        /// Constructors/Destructors
        ///////////////////////////////////////////////////////////////////////

        /**
         * @brief Construct a new Mqtt Client object
         *
         * @param brokerUri The URI of the broker to connect to
         */
        MqttClient(std::string brokerUri)
        {
            // Create the mqtt configuration
            ESP_LOGI(TAG, "creating mqtt configuration");
            esp_mqtt_client_config_t mqtt_cfg = {
                .broker = {
                    .address = {
                        .uri = brokerUri.c_str(),
                    }},
                .network = {.disable_auto_reconnect = true}};

            // Initialize the mqtt client
            ESP_LOGI(TAG, "initializing mqtt client");
            client = esp_mqtt_client_init(&mqtt_cfg);

            // Register the event handler
            // FIXME: We should be using results here instead. ESP_ERROR_CHECK will crash the program if the error is not ESP_OK
            ESP_LOGI(TAG, "registering mqtt event handler");
            ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqttEventHandler, this)); // No events will be dispatched from this
        }

        /**
         * @brief Destroy the Mqtt Client object
         */
        ~MqttClient()
        {
            // TODO: Do we need to unsubscribe from topics here?

            // Disconnect the client
            Result result = disconnect();
            if (!result.isSuccess())
            {
                ESP_LOGW(TAG, "failed to disconnect mqtt client: %s", result.getError().c_str());
            }

            // Unregister the event handler
            ESP_LOGI(TAG, "unregistering mqtt event handler");
            esp_err_t unregisterError = esp_mqtt_client_unregister_event(client, MQTT_EVENT_ANY, mqttEventHandler);
            if (unregisterError != ESP_OK)
            {
                ESP_LOGW(TAG, "failed to unregister mqtt event handler, error code: %s", esp_err_to_name(unregisterError));
            }

            // Destroy the mqtt client
            ESP_LOGI(TAG, "destroying mqtt client");
            esp_err_t destroyError = esp_mqtt_client_destroy(client);
            if (destroyError != ESP_OK)
            {
                ESP_LOGW(TAG, "failed to destroy mqtt client, error code: %s", esp_err_to_name(destroyError));
            }

            client = NULL;
        }

        ///////////////////////////////////////////////////////////////////////
        /// Connection/Disconnect
        ///////////////////////////////////////////////////////////////////////

        /**
         * @brief Connect to the broker
         */
        void connect()
        {
            // TODO: Consider if we should return an esp_err_t from this function (probably a result actually)
            // TODO: Check the current connection state before connecting and act accordingly
            // Start the mqtt client
            ESP_LOGI(TAG, "starting mqtt client");

            // Set the connection state to connecting
            setConnectionState(ConnectionState::CONNECTING);

            // Start the mqtt client
            esp_err_t error = esp_mqtt_client_start(client); // This will immediately cause a MQTT_EVENT_BEFORE_CONNECT event to be dispatched
            if (error != ESP_OK)
            {
                // Revert the connection state
                setConnectionState(ConnectionState::NOT_CONNECTED);

                ESP_LOGW(TAG, "failed to start mqtt client, error code: %s", esp_err_to_name(error));
                return;
            }
        }

        /**
         * @brief Disconnect from the broker
         * Note: This will not trigger a MQTT_EVENT_DISCONNECTED event.
         *
         * @return Result<> The result of the operation
         */
        Result<> disconnect()
        {
            // Check if we are already disconnected
            if (getConnectionState() == ConnectionState::NOT_CONNECTED)
            {
                ESP_LOGI(TAG, "already disconnected");
                return Result<>::createSuccess();
            }

            // Stop the mqtt client
            ESP_LOGI(TAG, "stopping mqtt client");
            esp_err_t error = esp_mqtt_client_stop(client);

            // Set the connection state to disconnected
            setConnectionState(ConnectionState::NOT_CONNECTED);

            return error == ESP_OK ? Result<>::createSuccess() : Result<>::createFailure(format("Failed to stop mqtt client, error code: %s", esp_err_to_name(error)));
        }

        /**
         * @brief Reconnect the client
         *
         * @return Result<> The result of the operation
         */
        Result<> reconnect()
        {
            // Set the connection state to connecting
            setConnectionState(ConnectionState::CONNECTING);

            // Attempt to reconnect the client
            esp_err_t error = esp_mqtt_client_reconnect(client);
            if (error != ESP_OK)
            {
                // Revert the connection state
                setConnectionState(ConnectionState::NOT_CONNECTED);

                return Result<>::createFailure(format("Failed to reconnect mqtt client, error code: %s", esp_err_to_name(error)));
            }

            return Result<>::createSuccess();
        }

        ///////////////////////////////////////////////////////////////////////
        /// Update the broker URI
        ///////////////////////////////////////////////////////////////////////

        /**
         * @brief Set the broker URI. This will disconnect the client if it is already connected. However, it will not reconnect.
         *
         * @param brokerUri The URI of the broker to connect to
         */
        void setBrokerUri(std::string brokerUri)
        {
            // Disconnect the client
            disconnect();

            // Set the new broker URI
            esp_mqtt_client_set_uri(client, brokerUri.c_str());
        }

        ///////////////////////////////////////////////////////////////////////
        /// Get or wait for a connection state
        ///////////////////////////////////////////////////////////////////////

        /**
         * @brief Get the current connection state
         *
         * @return ConnectionState The current connection state
         */
        ConnectionState getConnectionState()
        {
            return connectionState.load(std::memory_order_relaxed);
        }

        /**
         * @brief Wait for a specific connection state
         *
         * @param connectionState The desired connection state
         * @param timeoutMs The timeout in milliseconds
         * @return true The connection state is the desired state
         * @return false The connection state is not the desired state
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

        ///////////////////////////////////////////////////////////////////////
        /// Subscribe/Unsubscribe
        ///////////////////////////////////////////////////////////////////////

        /**
         * @brief Subscribe to a topic
         *
         * @param topic The topic to subscribe to
         * @param publishHandler The handler for when a message is published to the topic
         * @return int The message ID of the subscribe message or -1 on failure -2 in case of full outbox
         */
        int subscribe(std::string topic, std::function<void(MqttClient *, int, std::vector<std::byte>)> publishHandler)
        {
            /**
             * Note: Whether or not this method blocks until subscribing is successful is unknown.
             *
             * It is unknown if this will method will appear to be successful when it actually isn't
             * The esp_mqtt_client_subscribe_single function returns before a MQTT_EVENT_SUBSCRIBED event is fired.
             * It may only be just before the event is fired though and this may block until that event is fired.
             *
             * Unsubscribing immediately after subscribing appears to be successful, though the events seem to be out of order,
             * leading me to believe that this does not block until subscriptions are successful.
             * EX order of events:
             *    Subscribing
             *    Unsubscribing
             *    Subscribed
             *    Unsubscribed
             */

            // Subscribe to the topic
            ESP_LOGI(TAG, "subscribing to topic: %s", topic.c_str());

            // Add the delegate to the map
            topicsToPublishHandlers[topic] = publishHandler;

            return esp_mqtt_client_subscribe_single(client, topic.c_str(), 0);
        }

        /**
         * @brief Unsubscribe from a topic
         *
         * @param topic The topic to unsubscribe from
         * @return int The message ID of the unsubscribe message or -1 on failure
         */
        int unsubscribe(std::string topic)
        {
            // Unsubscribe from the topic
            ESP_LOGI(TAG, "unsubscribing from topic: %s", topic.c_str());
            return esp_mqtt_client_unsubscribe(client, topic.c_str());
        }

        ///////////////////////////////////////////////////////////////////////
        /// Publishers
        ///////////////////////////////////////////////////////////////////////

        /**
         * @brief Publish a message to a topic
         *
         * @param topic The topic to publish to
         * @param data The data to publish
         * @return int The message ID of the publish message (for QoS 0 message_id will always be zero) or -1 on failure -2 in case of full outbox
         */
        int publish(std::string topic, std::vector<std::byte> data)
        {
            // TODO: Consider adding a QoS parameter
            // Publish the message
            ESP_LOGI(TAG, "publishing message to topic: %s", topic.c_str());
            return esp_mqtt_client_publish(client, topic.c_str(), reinterpret_cast<char *>(data.data()), data.size(), 0, 0);
        }

        /**
         * @brief Publish a message to a topic
         *
         * @param topic The topic to publish to
         * @param message The message to publish
         * @return int The message ID of the publish message (for QoS 0 message_id will always be zero) or -1 on failure -2 in case of full outbox
         */
        int publish(std::string topic, std::string message)
        {
            // TODO: Consider adding a QoS parameter
            // Publish the message
            ESP_LOGI(TAG, "publishing message to topic: %s", topic.c_str());
            return esp_mqtt_client_publish(client, topic.c_str(), message.c_str(), 0, 0, 0);
        }

    private:
        // Thread-safe connection state management variables
        std::atomic<ConnectionState> connectionState; // The current connection state
        std::condition_variable stateChanged;         // Condition variable for state change notifications
        std::mutex stateMutex;                        // Mutex to protect condition variable
        esp_mqtt_client_handle_t client;              // The MQTT client handle
        std::map<
            std::string,
            std::function<void(MqttClient *, int, std::vector<std::byte>)>>
            topicsToPublishHandlers; // Map of topics to publish handlers

        /**
         * @brief Set the connection state
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
         * @brief MQTT event handler
         *
         * @param handlerArgs The handler arguments (an MqttClient object pointer in this case)
         * @param base The event base
         * @param eventId The event ID
         * @param eventData The event data
         */
        static void mqttEventHandler(void *handlerArgs, esp_event_base_t base, int32_t eventId, void *eventData)
        {
            // Log the event
            ESP_LOGI(TAG, "event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, eventId);

            // Convert the handler args to a MqttClient pointer
            MqttClient *mqttClient = (MqttClient *)handlerArgs;

            // Get the event
            esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)eventData;

            // Get the topic
            std::string topic(event->topic, event->topic_len);

            switch ((esp_mqtt_event_id_t)eventId)
            {
            case MQTT_EVENT_BEFORE_CONNECT:
                // Client has initialized and is about to start connecting to the broker
                ESP_LOGI(TAG, "connecting to broker");
                break;
            case MQTT_EVENT_CONNECTED:
                // Client has connected to the broker
                ESP_LOGI(TAG, "connected to broker");

                // Set the connection state to connected
                mqttClient->setConnectionState(ConnectionState::CONNECTED);

                // Check if the onConnected delegate is set
                if (mqttClient->onConnected == nullptr)
                {
                    break;
                }

                // Call the onConnected delegate
                mqttClient->onConnected(mqttClient);
                break;
            case MQTT_EVENT_DISCONNECTED:
                // Client has disconnected from the broker
                ESP_LOGI(TAG, "disconnected from broker");

                // Disconnect the client
                esp_mqtt_client_disconnect(mqttClient->client);

                // Set the connection state to disconnected
                mqttClient->setConnectionState(ConnectionState::NOT_CONNECTED);

                // Check if the onDisconnected delegate is set
                if (mqttClient->onDisconnected == nullptr)
                {
                    break;
                }

                // Spawn the onDisconnect delegate on a new task
                xTaskCreate(
                    [](void *mqttClientPointer)
                    {
                        MqttClient *mqttClient = (MqttClient *)mqttClientPointer;
                        mqttClient->onDisconnected(mqttClient);
                        vTaskDelete(NULL);
                    },
                    "onDisconnect",
                    4096,
                    mqttClient,
                    5,
                    nullptr);
                break;
            case MQTT_EVENT_SUBSCRIBED:
                // Client has subscribed to a topic
                ESP_LOGI(TAG, "subscribed to topic, msg_id=%d", event->msg_id);

                /**
                 * Note: This event does not include the topic we have subscribed to. It does however include the message ID.
                 * This is the same message ID that is returned from the esp_mqtt_client_subscribe_single function,
                 * so in theory we could determine which topic we have just subscribed to, but this has not been implemented.
                 */
                break;
            case MQTT_EVENT_UNSUBSCRIBED:
                // Client has unsubscribed from a topic
                ESP_LOGI(TAG, "unsubscribed from topic %s, msg_id=%d", topic.c_str(), event->msg_id);
                break;
            case MQTT_EVENT_PUBLISHED:
                // Client has published a message
                ESP_LOGI(TAG, "published message to topic %s, msg_id=%d", topic.c_str(), event->msg_id);

                /**
                 * Note: This event is fired when the broker acknowledges that this client has successfully published a message.
                 * The broker does not send acknowledgements to the client if the QoS level is 0, set by the esp_mqtt_client_publish function,
                 * hence this event will not be fired if the QoS level is 0.
                 */
                break;
            case MQTT_EVENT_DATA:
            {
                // Client has received data
                ESP_LOGI(TAG, "received data on topic %s, msg_id=%d", topic.c_str(), event->msg_id);

                // Find the topic handler in the map
                std::function<void(MqttClient *, int, std::vector<std::byte>)> topicHandler = mqttClient->topicsToPublishHandlers[topic];

                // Verify the handler exists
                if (topicHandler == nullptr)
                {
                    ESP_LOGW(TAG, "no handler found for topic %s", topic.c_str());
                    break;
                }

                // Get the event data and convert it to a byte vector
                std::vector<std::byte> data(reinterpret_cast<std::byte *>(event->data),
                                            reinterpret_cast<std::byte *>(event->data) + event->data_len);

                // Call the handler
                topicHandler(mqttClient, event->msg_id, data);
                break;
            }
            case MQTT_EVENT_DELETED:
                // Client has deleted a message
                ESP_LOGW(TAG, "deleted message, msg_id=%d", event->msg_id);
                break;
            default:
                ESP_LOGW(TAG, "unhandled MQTT event id: %d", event->event_id);
                break;
            }
        }
    };
};