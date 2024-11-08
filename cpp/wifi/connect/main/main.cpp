#include <string.h>
#include <inttypes.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "include/wifi_service.h"

static const char *TAG = "wifi station";
static uint8_t maxRetries = 5;

void onConnect() {
    ESP_LOGI(TAG, "connected to wifi network");
}

void onDisconnect() {
    // Attempt to reconnect to the access point
    if (maxRetries > 0) {
        ESP_LOGI(TAG, "disconnected from wifi network, attempting to reconnect");
        WifiService::startConnect(ApCredentialInfo(
            "denhac",
            "denhac rules"
        ));
        maxRetries--;
    } else {
        ESP_LOGI(TAG, "disconnected from wifi network, maximum retries reached");
    }
}

extern "C" void app_main(void)
{
    // Initiate the wifi service
    ESP_LOGI(TAG, "initializing wifi service");
    WifiService::init();

    // Set the onConnect and onDisconnect delegates
    WifiService::onConnect = onConnect;
    WifiService::onDisconnect = onDisconnect;

    // Start connecting to an access point
    WifiService::startConnect(ApCredentialInfo(
        "denhac",
        "denhac rules"
    ));
}
