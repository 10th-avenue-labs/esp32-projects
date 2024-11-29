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

extern "C" void app_main(void)
{
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
            std::string message = "Hello, World!";

            // Convert the message to a vector of bytes and send the data
            return std::vector<std::byte>(
                reinterpret_cast<const std::byte*>(message.data()),
                reinterpret_cast<const std::byte*>(message.data()) + message.size()
            );
        },
        false                                   // acknowledgeWrites
    );

    // Create a BLE service
    std::shared_ptr<BleService> service = std::make_shared<BleService>(
        "10000000-0000-0000-0000-000000000000", // UUID
        std::vector<std::shared_ptr<BleCharacteristic>>{characteristic}
    );

    // Need to support this functionality (implement copy constructor for characteristics)
    // BleService service2(
    //     "00001801-0000-1000-8000-00805f9b34fb", // UUID
    //     {
    //         BleCharacteristic(
    //             "00002a05-0000-1000-8000-00805f9b34fb", // UUID
    //             nullptr,                                // onWrite
    //             nullptr,                                // onRead
    //             false                                   // acknowledgeWrites
    //         )
    //     }
    // );

    // Initialize the BLE advertiser
    BleAdvertiser::init(
        "ADT Service",                          // Name
        BLE_GAP_APPEARANCE_GENERIC_TAG,         // Appearance
        BLE_GAP_LE_ROLE_PERIPHERAL,             // Role
        {service}                               // Services
    );

    // Start advertising
    BleAdvertiser::advertise();
}
