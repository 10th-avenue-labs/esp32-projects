#include <unordered_map>
#include <string>
#include <functional>
#include <cstring>
#include <vector>
#include <ranges>
#include <algorithm>
#include <atomic>
#include "ap_scan_results.h"
#include "ap_credential_info.h"

extern "C"{
    #include <esp_log.h>
    #include <esp_err.h>
    #include <nvs_flash.h>
    #include <esp_netif.h>
    #include <esp_wifi_default.h>
    #include <esp_event.h>
    #include <esp_wifi.h>
}

enum ConnectionState {
    NOT_CONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
};

class WifiService {
public:
    static std::function<void(void)> onConnect;
    static std::function<void(void)> onDisconnect;

    ////////////////////////////////////////////////////////////////////////////
    // Initialization and Disposal
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Initialize the WiFi service.
     * 
     * @return true if the WiFi service was successfully initialized, false otherwise.
     */
    static bool init();

    // TODO: Dispose method

    ////////////////////////////////////////////////////////////////////////////
    // Access Point Scanning
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Scan for available access points.
     * 
     * @param maxApsCount The maximum number of access points to scan for.
     * @return ApScanResults The results of the scan.
     */
    static ApScanResults scanAvailableAccessPoints(uint8_t maxApsCount);

    ////////////////////////////////////////////////////////////////////////////
    // Connection Management
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Connect to an access point.
     * 
     * @param apCredentialInfo The access point credentials.
     * @return true if the connection was successfully started, false otherwise.
     */
    static bool startConnect(
        ApCredentialInfo apCredentialInfo
    );

    /**
     * @brief Disconnect from the current access point.
     * 
     * @return true if the disconnection was successfully started, false otherwise.
     */
    static bool startDisconnect();

    ////////////////////////////////////////////////////////////////////////////
    // Connection State Management
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Get the current connection state.
     * 
     * @return ConnectionState The current connection state.
     */
    static ConnectionState getConnectionState();

    /**
     * @brief Set the connection state.
     * 
     * @param newState The new connection state.
     */
    static void setConnectionState(ConnectionState newState);

    /**
     * @brief Wait for the connection state to be one of the specified states.
     * 
     * @param connectionStates The connection states to wait for.
     */
    static void waitConnectionState(std::vector<ConnectionState> connectionStates);

private:
    static std::atomic<ConnectionState> connectionState;

    ////////////////////////////////////////////////////////////////////////////
    // Event Handlers
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Handle generic events and delegate them to more specific event handlers.
     * 
     * @param arg Arguments passed to the generic event handler.
     * @param eventBase The event base, eg wifi event or IP event.
     * @param eventId The event ID, eg wifi ready or IP obtained.
     * @param eventData The data associated with the event, eg an IP address for an IP obtained event.
     */
    static void genericEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);

    /**
     * @brief Handle WiFi events.
     * 
     * @param eventId The specific WiFi event, eg WiFi ready or WiFi disconnected.
     * @param eventData The data associated with the WiFi event.
     */
    static void wifiEventHandler(int32_t eventId, void* eventData);

    /**
     * @brief Handle IP events.
     * 
     * @param eventId The specific IP event, eg IP obtained.
     * @param eventData The data associated with the IP event.
     */
    static void ipEventHandler(int32_t eventId, void* eventData);
};