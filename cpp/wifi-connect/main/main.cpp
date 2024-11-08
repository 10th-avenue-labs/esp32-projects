#include <string.h>
#include <inttypes.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "include/wifi_service.h"

static const char *TAG = "wifi station";

void onConnect() {
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX connected to an access point");
}

void onDisconnect() {
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX disconnected from an access point");
}

extern "C" void app_main(void)
{
    // Initiate the wifi service
    ESP_LOGI(TAG, "initializing wifi service");
    WifiService::init();

    // Set the onConnect and onDisconnect delegates
    WifiService::onConnect = onConnect;
    WifiService::onDisconnect = onDisconnect;

    // Connect to an access point again
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX CONNECT -1");
    bool success = WifiService::startConnect(
        ApCredentialInfo {
            "denhac",
            "denhac rules"
        }
    );
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX connect success: %s", success ? "true" : "false");

    // Wait until we're connected to initiate a new wifi scan
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX WAIT -1");
    WifiService::waitConnectionState({ConnectionState::CONNECTED, ConnectionState::NOT_CONNECTED});
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX connection_state: %d", WifiService::getConnectionState());

    // Disconnect from the access point
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX DISCONNECT -1");
    WifiService::startDisconnect();

    // Connect to an access point again
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX CONNECT 0");
    success = WifiService::startConnect(
        ApCredentialInfo {
            "denhac",
            "denhac rules"
        }
    );
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX connect success: %s", success ? "true" : "false");

    // Wait until we're connected to initiate a new wifi scan
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX WAIT 0");
    WifiService::waitConnectionState({ConnectionState::CONNECTED, ConnectionState::NOT_CONNECTED});
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX connection_state: %d", WifiService::getConnectionState());

    // Disconnect from the access point
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX DISCONNECT -1");
    WifiService::startDisconnect();

    // Scan for available access points (this does not need to be done to directly connect to an access point)
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX SCAN 1");
    ApScanResults scanResults = WifiService::scanAvailableAccessPoints(30);

    // Log the scan results
    ESP_LOGI(TAG, "total number of access points found: %d, number of access points populated: %d", scanResults.totalAccessPointsFound, scanResults.accessPoints.size());

    // Connect to an access point
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX CONNECT 1");
    success = WifiService::startConnect(
        ApCredentialInfo {
            "IP-in-the-hot-tub",
            "everytime"
        }
    );
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX connect success: %s", success ? "true" : "false");

    // Wait until we're connected to initiate a new wifi scan   
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX WAIT 1");
    WifiService::waitConnectionState({ConnectionState::CONNECTED, ConnectionState::NOT_CONNECTED});
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX connection_state: %d", WifiService::getConnectionState());

    // Disconnect from the access point
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX DISCONNECT -1");
    WifiService::startDisconnect();

    // Scan for available access points (this does not need to be done to directly connect to an access point)
    // We can not try to scan for access points while the wifi station is in the process of connecting
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX SCAN 2");
    scanResults = WifiService::scanAvailableAccessPoints(30);

    // Log the scan results
    ESP_LOGI(TAG, "total number of access points found: %d, number of access points populated: %d", scanResults.totalAccessPointsFound, scanResults.accessPoints.size());
    for(int i = 0; i < scanResults.accessPoints.size(); i++) {
        // ESP_LOGI(TAG, "access point number %d", i);
        // ESP_LOGI(TAG, "\tssid: %s", scanResults.accessPoints[i].ssid.c_str());
        // ESP_LOGI(TAG, "\trssi: %d", scanResults.accessPoints[i].rssi);
        // ESP_LOGI(TAG, "\tauth mode: %d", scanResults.accessPoints[i].authMode);
        // ESP_LOGI(TAG, "\tchannel: %d", scanResults.accessPoints[i].channel);
    }

    // Connect to an access point again
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX CONNECT 2");
    success = WifiService::startConnect(
        ApCredentialInfo {
            "IP-in-the-hot-tub",
            "everytime"
        }
    );
    ESP_LOGI(TAG, "XXXXXXXXXXXXXXXXXXXXXXXX connect success: %s", success ? "true" : "false");

    ESP_LOGI(TAG, "done");

    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
