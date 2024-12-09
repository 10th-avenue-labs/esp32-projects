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

#define ADT_SERVICE_UUID                                "cc3aab60-0001-0000-81e0-c88f19fb28cb"
#define ADT_SERVICE_MTU_CHARACTERISTIC_UUID             "cc3aab60-0001-0001-81e0-c88f19fb28cb"
#define ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID    "cc3aab60-0001-0002-81e0-c88f19fb28cb"
#define ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID         "cc3aab60-0001-0003-81e0-c88f19fb28cb"

static void bleAdvertiserTask(void* args) {
    // Advertise the Ble Device
    ESP_LOGI(TAG, "advertising BLE Device");
    BleAdvertiser::advertise();

    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    bool* sendTransfer = new bool(false);
    uint16_t* messageId = new uint16_t(0);
    shared_ptr<BleDevice> device;

    AdtService adtService(
        ADT_SERVICE_UUID,
        ADT_SERVICE_MTU_CHARACTERISTIC_UUID,
        ADT_SERVICE_TRANSMISSION_CHARACTERISTIC_UUID,
        ADT_SERVICE_RECEIVE_CHARACTERISTIC_UUID,
        [messageId, sendTransfer, &device](uint16_t sendingMessageId, std::vector<std::byte> data, shared_ptr<BleDevice> sendingDevice) {
            ESP_LOGI(TAG, "message received of size %d", data.size());

            // Convert the data to a string
            std::string message(
                reinterpret_cast<const char*>(data.data()),
                reinterpret_cast<const char*>(data.data()) + data.size()
            );

            // Log the message
            ESP_LOGI(TAG, "message: %s", message.c_str());

            // Set the lambda variables
            *messageId = sendingMessageId;
            *sendTransfer = true;
            device = sendingDevice;
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

            ESP_LOGI(TAG, "responding to message %d", *messageId);

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

            adtService.sendMessage({device}, data);

            *sendTransfer = false;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Note: Reads will also change a value in the characteristic. A Notify is essentially a read without the central taking the first action