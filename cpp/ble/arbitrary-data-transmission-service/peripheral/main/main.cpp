#include <algorithm>
#include <string>
#include "BleAdvertiser.h"
#include "BleService.h"
#include "AdtService.h"
#include "BleCharacteristic.h"

extern "C"
{
    #include <string.h>
    #include <inttypes.h>
    #include "esp_log.h"
    #include <host/ble_uuid.h>
    #include <esp_err.h>
}

static const char *TAG = "adt service";

// BLE constants
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

static void bleAdvertiserTask(void* args) {
    // Advertise the Ble Device
    ESP_LOGI(TAG, "advertising BLE Device");
    BleAdvertiser::advertise();

    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    bool* sendTransfer = new bool(false);

    AdtService adtService(
        "00000000-0000-0000-0000-000000012346",             // Service UUID
        "12345670-0000-0000-0000-000000000000",             // MTU Characteristic UUID
        "12345679-0000-0000-0000-000000000000",             // Transmission Characteristic UUID
        "12345678-0000-0000-0000-000000000000",             // Receive Characteristic UUID
        [sendTransfer](std::vector<std::byte> data) {                   // onMessageReceived
            ESP_LOGI(TAG, "message received of size %d", data.size());

            // Convert the data to a string
            std::string message(
                reinterpret_cast<const char*>(data.data()),
                reinterpret_cast<const char*>(data.data()) + data.size()
            );

            // Log the message
            ESP_LOGI(TAG, "message: %s", message.c_str());

            // Set the transfer flag
            *sendTransfer = true;
        }
    );

    // Initialize the BLE advertiser
    BleAdvertiser::init(
        "ADT Service",                          // Name
        BLE_GAP_APPEARANCE_GENERIC_TAG,         // Appearance
        BLE_GAP_LE_ROLE_PERIPHERAL,             // Role
        {adtService.getBleService()},           // Services
        [](shared_ptr<BleDevice> device){       // onDeviceConnected
            ESP_LOGI(TAG, "Device connected");
        }
    );

    // Create a task for the BLE advertiser
    xTaskCreate(bleAdvertiserTask, "ble_advertiser", 4096, NULL, 5, NULL);

    while(1) {
        // Check if the device is connected
        if (*sendTransfer) {
            // Wait for a time while the connection finishes
            vTaskDelay(3000 / portTICK_PERIOD_MS);

            // Create a message to sent to the device
            std::string partD = std::string(249, 'd');
            std::string partE = std::string(249, 'e');
            std::string partF = std::string(249, 'f');
            std::string message = partD + partE + partF;

            // Convert the message to a vector of bytes
            std::vector<std::byte> data(
                reinterpret_cast<const std::byte*>(message.data()),
                reinterpret_cast<const std::byte*>(message.data()) + message.size()
            );

            adtService.sendMessage({BleAdvertiser::connectedDevicesByHandle.begin()->second}, data);

            *sendTransfer = false;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Note: Reads will also change a value in the characteristic. A Notify is essentially a read without the central taking the first action