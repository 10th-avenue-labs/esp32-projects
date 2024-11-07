#include <string>

extern "C"{
    #include <cstdint>
    #include <esp_wifi_types_generic.h>
}

class ApScanResult {
public:
    std::string ssid;
    int8_t rssi;
    wifi_auth_mode_t authMode;
    uint8_t channel;
};