#include "WifiService.h"
#include "BleAdvertiser.h"

extern "C" {
    #include <string.h>
    #include <inttypes.h>
    #include "esp_wifi.h"
    #include "esp_event.h"
    #include "esp_log.h"
}

static const char *TAG = "wifi station";

extern "C" void app_main(void)
{
    // Initiate the wifi service
    ESP_LOGI(TAG, "initializing wifi service");
    WifiService::init();

    // Scan for available access points (this does not need to be done to directly connect to an access point)
    ApScanResults scanResults = WifiService::scanAvailableAccessPoints(30);

    // Log the scan results
    ESP_LOGI(TAG, "total number of access points found: %d, number of access points populated: %d", scanResults.totalAccessPointsFound, scanResults.accessPoints.size());
    for(int i = 0; i < scanResults.accessPoints.size(); i++) {
        ESP_LOGI(TAG, "access point number %d", i);
        ESP_LOGI(TAG, "\tssid: %s", scanResults.accessPoints[i].ssid.c_str());
        ESP_LOGI(TAG, "\trssi: %d", scanResults.accessPoints[i].rssi);
        ESP_LOGI(TAG, "\tauth mode: %d", scanResults.accessPoints[i].authMode);
        ESP_LOGI(TAG, "\tchannel: %d", scanResults.accessPoints[i].channel);
    }

    ESP_LOGI(TAG, "finished scan");
}
