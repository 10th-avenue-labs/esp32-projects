#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <string>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <map>
#include <functional>

extern "C"
{
#include <mqtt_client.h>
#include <esp_log.h>
}

namespace Mqtt
{

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
        std::function<void(MqttClient*)> onConnected;       // Delegate for when the client is connected
        std::function<void(MqttClient*)> onDisconnected;    // Delegate for when the client is disconnected

        ///////////////////////////////////////////////////////////////////////
        /// Constructors/Destructors
        ///////////////////////////////////////////////////////////////////////

        /**
         * @brief Construct a new Mqtt Client object
         * 
         * @param brokerUri The URI of the broker to connect to
         */
        MqttClient(std::string brokerUri);

        /**
         * @brief Destroy the Mqtt Client object
         */
        ~MqttClient();

        ///////////////////////////////////////////////////////////////////////
        /// Connection/Disconnect
        ///////////////////////////////////////////////////////////////////////

        /**
         * @brief Connect to the broker
         */
        void connect();

        /**
         * @brief Disconnect from the broker
         */
        void disconnect();

        ///////////////////////////////////////////////////////////////////////
        /// Get or wait for a connection state
        ///////////////////////////////////////////////////////////////////////

        /**
         * @brief Get the current connection state
         * 
         * @return ConnectionState The current connection state
         */
        ConnectionState getConnectionState();

        /**
         * @brief Wait for a specific connection state
         * 
         * @param connectionState The desired connection state
         * @param timeoutMs The timeout in milliseconds
         * @return true The connection state is the desired state
         * @return false The connection state is not the desired state
         */
        bool waitConnectionState(const std::vector<ConnectionState> &connectionStates, int timeoutMs = -1);

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
        int subscribe(std::string topic, std::function<void(MqttClient*, int, std::vector<std::byte>)> publishHandler);

        /**
         * @brief Unsubscribe from a topic
         * 
         * @param topic The topic to unsubscribe from
         * @return int The message ID of the unsubscribe message or -1 on failure
         */
        int unsubscribe(std::string topic);

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
        int publish(std::string topic, std::vector<std::byte> data);

        /**
         * @brief Publish a message to a topic
         * 
         * @param topic The topic to publish to
         * @param message The message to publish
         * @return int The message ID of the publish message (for QoS 0 message_id will always be zero) or -1 on failure -2 in case of full outbox
         */
        int publish(std::string topic, std::string message);
    private:
        // Thread-safe connection state management variables
        std::atomic<ConnectionState> connectionState;   // The current connection state
        std::condition_variable stateChanged;           // Condition variable for state change notifications
        std::mutex stateMutex;                          // Mutex to protect condition variable
        esp_mqtt_client_handle_t client;                // The MQTT client handle
        std::map<
            std::string,
            std::function<void(MqttClient*, int, std::vector<std::byte>)>
        > topicsToPublishHandlers;                      // Map of topics to publish handlers

        /**
         * @brief Set the connection state
         * 
         * @param newState The new connection state
         */
        void setConnectionState(ConnectionState newState);

        /**
         * @brief MQTT event handler
         * 
         * @param handlerArgs The handler arguments (an MqttClient object pointer in this case)
         * @param base The event base
         * @param eventId The event ID
         * @param eventData The event data
         */
        static void mqttEventHandler(void *handlerArgs, esp_event_base_t base, int32_t eventId, void *eventData);
    };
};

#endif // MQTT_CLIENT_H