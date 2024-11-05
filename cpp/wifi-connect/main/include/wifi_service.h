#include <unordered_map>
#include <string>
#include <functional>
#include <cstring>

extern "C"{
    #include <esp_log.h>
    #include <esp_err.h>
    #include <nvs_flash.h>
    #include <esp_netif.h>
    #include <esp_wifi_default.h>
    #include <esp_event.h>
    #include <esp_wifi.h>
}

class WifiService {
public:
    static bool init(std::string ssid, std::string password);
private:

    static void genericEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);
    static void wifiEventHandler(void* arg, int32_t eventId, void* eventData);
    static void ipEventHandler(void* arg, int32_t eventId, void* eventData);
};