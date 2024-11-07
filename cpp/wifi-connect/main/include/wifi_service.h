#include <unordered_map>
#include <string>
#include <functional>
#include <cstring>
#include <vector>
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
    CONNECTED
};

class WifiService {
public:
    static bool init();
    static ApScanResults scanAvailableAccessPoints(uint8_t maxApsCount);
    static bool startConnect(
        ApCredentialInfo apCredentialInfo,
        std::function<void(void)> onConnect,
        std::function<void(void)> onDisconnect
    );
    // disconnect
    // dispose
    // Blocking connect method maybe?
    static ConnectionState getConnectionState();
    static void waitConnectionState(ConnectionState connectionState);
private:
    static ConnectionState connectionState;

    static void genericEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);
    static void wifiEventHandler(void* arg, int32_t eventId, void* eventData);
    static void ipEventHandler(void* arg, int32_t eventId, void* eventData);
};