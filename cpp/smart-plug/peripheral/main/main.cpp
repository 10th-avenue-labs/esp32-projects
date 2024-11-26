#include "WifiService.h"
#include "BleAdvertiser.h"
#include "Timer.h"
#include "AcDimmer.h"
#include "MqttClient.h"
#include "AdtService.h"
#include "include/smart_plug.h"

extern "C" {
    #include <string.h>
    #include <inttypes.h>
    #include "esp_wifi.h"
    #include "esp_event.h"
    #include "esp_log.h"
}

static const char *TAG = "smart plug";

// BLE constants
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

// Define the BLE UUIDs for the ADT service
#define ADT_SERVICE         "6b0a95b9-0000-44d9-957c-1f7cd56563b4"
#define ADT_MTU_CHAR        "6b0a95b9-0010-44d9-957c-1f7cd56563b4"
#define ADT_TRANSMIT_CHAR   "6b0a95b9-0020-44d9-957c-1f7cd56563b4"

void adtMessageReceivedHandler(std::vector<std::byte> data) {
    ESP_LOGI(TAG, "received ADT message");

    // Convert the data to a string
    std::string message(reinterpret_cast<const char *>(data.data()), data.size());

    // Log the message
    ESP_LOGI(TAG, "message: %s", message.c_str());
}

void bleInit(AcDimmer* acDimmer) {
    // Create the ADT service
    ESP_LOGI(TAG, "creating ADT service");
    AdtService* adtService = new AdtService(
        ADT_SERVICE,
        ADT_MTU_CHAR,
        ADT_TRANSMIT_CHAR,
        adtMessageReceivedHandler
    );

    // Initialize the Ble Advertiser
    ESP_LOGI(TAG, "initializing BLE Advertiser");
    BleAdvertiser::init("Smart Plug", BLE_GAP_APPEARANCE_GENERIC_TAG, BLE_GAP_LE_ROLE_PERIPHERAL, {*adtService->getBleService()});
}

static void bleAdvertiserTask(void* args) {
    // Advertise the Ble Device
    ESP_LOGI(TAG, "advertising BLE Device");
    BleAdvertiser::advertise();

    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    // TODO: Implement NVS
    // Attempt to read the smart plug brightness from NVS
    // Attempt to read the wifi configuration from NVS
    // Attempt to read the mqtt configuration from NVS
    // Attempt to read the schedule from NVS

    // Initiate the ac dimmer
    // Config for non-dimmable LED
    AcDimmer acDimmer(
        32,     // ZC pin
        25,     // PSM pin
        1000,   // Debounce time
        5000,   // Offset leading (this would change for dimmable lights)
        1200    // Offset falling (this would change for dimmable lights)
    );

    // Initiate the Ble Advertiser
    bleInit(&acDimmer);

    // Create a task for the BLE advertiser
    xTaskCreate(bleAdvertiserTask, "ble_advertiser", 4096, NULL, 5, NULL);
}
