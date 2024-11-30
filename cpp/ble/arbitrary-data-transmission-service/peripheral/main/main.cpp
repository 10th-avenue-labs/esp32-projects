#include <algorithm>
#include <string>
#include "BleAdvertiser.h"
#include "BleService.h"
// #include "AdtService.h"
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
    bool* deviceSubscribed = new bool(false);

    // Create a BLE characteristic
    std::shared_ptr<BleCharacteristic> characteristic = std::make_shared<BleCharacteristic>(
        "10000000-1000-0000-0000-000000000000", // UUID
        [](std::vector<std::byte> data){        // onWrite
            ESP_LOGI(TAG, "onWrite called with data length %d", data.size());

            // Convert the data to a string
            std::string message(reinterpret_cast<const char *>(data.data()), data.size());

            // Print the data
            ESP_LOGI(TAG, "Received: %s", message.c_str());

            // Return success
            return 0;
        },
        [](){                                   // onRead
            ESP_LOGI(TAG, "onRead called");

            // Create a message
            std::string message = "Hello, World from peripheral!";

            // Convert the message to a vector of bytes and send the data
            return std::vector<std::byte>(
                reinterpret_cast<const std::byte*>(message.data()),
                reinterpret_cast<const std::byte*>(message.data()) + message.size()
            );
        },
        [deviceSubscribed](shared_ptr<BleDevice> device){
            ESP_LOGI(TAG, "onSubscribe called");
            *deviceSubscribed = true;
        },                                 // onSubscribe
        false                                   // acknowledgeWrites
    );

    // Create a BLE service
    std::shared_ptr<BleService> service = std::make_shared<BleService>(
        "10000000-0000-0000-0000-000000000000", // UUID
        std::vector<std::shared_ptr<BleCharacteristic>>{characteristic}
    );

    // Initialize the BLE advertiser
    BleAdvertiser::init(
        "ADT Service",                          // Name
        BLE_GAP_APPEARANCE_GENERIC_TAG,         // Appearance
        BLE_GAP_LE_ROLE_PERIPHERAL,             // Role
        {service},                              // Services
        [](shared_ptr<BleDevice> device){                   // onDeviceConnected
            ESP_LOGI(TAG, "Device connected");
        }
    );

    // Create a task for the BLE advertiser
    xTaskCreate(bleAdvertiserTask, "ble_advertiser", 4096, NULL, 5, NULL);

    while(1) {
        // Check if the device is connected
        if (*deviceSubscribed) {
            // Wait for a time while the connection finishes
            vTaskDelay(5000 / portTICK_PERIOD_MS);

            // Create a message to sent to the device
            std::string message = "Hello, World from main!";

            // Convert the message to a vector of bytes
            std::vector<std::byte> data(
                reinterpret_cast<const std::byte*>(message.data()),
                reinterpret_cast<const std::byte*>(message.data()) + message.size()
            );

            // Notify the device
            characteristic->notify({BleAdvertiser::connectedDevicesByHandle.begin()->second}, data);

            *deviceSubscribed = false;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


// Note: Reads will also change a value in the characteristic. A Notify is essentially a read without the central taking the first action