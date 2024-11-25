#include <algorithm>
#include <string>
#include "BleAdvertiser.h"
#include "BleService.h"
#include "AdtService.h"

extern "C" {
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

int onWrite(std::vector<std::byte> data) {
    ESP_LOGI(TAG, "reading data of size %d", data.size());

    // Get the event type
    uint8_t eventType = std::to_integer<int>(data[0]);

    // Get the total chunks (for start event types) or chunk number (for data event types)
    uint8_t chunkNumber = std::to_integer<uint8_t>(data[1]);

    // Get the message ID
    uint16_t result = (static_cast<uint16_t>(std::to_integer<uint8_t>(data[3])) << 8) |
                      static_cast<uint16_t>(std::to_integer<uint8_t>(data[2]));

    // Convert the data to a string
    std::string str(reinterpret_cast<const char*>(data.data() + 4), data.size() - 4);

    // Print the header information
    ESP_LOGI(TAG, "event type: %d, chunk number: %d, message ID: %d", eventType, chunkNumber, result);

    // Print the data
    ESP_LOGI(TAG, "received data: %s", str.c_str());
    return 0;
};

std::vector<std::byte> onRead(void) {
    ESP_LOGI(TAG, "onRead");

    // Get the MTU
    uint16_t mtu = BleAdvertiser::getMtu();

    // Convert the MTU to a vector of bytes
    std::vector<std::byte> mtuBytes;
    for (int i = 0; i < sizeof(mtu); i++) {
        mtuBytes.push_back(static_cast<std::byte>((mtu >> (i * 8)) & 0xFF));
    }

    return mtuBytes;
};

extern "C" void app_main(void)
{
    // Call init with inline map initialization
    BleAdvertiser::init("Dimmer", BLE_GAP_APPEARANCE_GENERIC_TAG, BLE_GAP_LE_ROLE_PERIPHERAL, {
        {"00000000-0000-0000-0000-000000012345", {BleCharacteristic("12345678-0000-0000-0000-000000000000", nullptr, nullptr, true, true, false)}},
        // Arbitrary data transfer service
        {"00000000-0000-0000-0000-000000012346", 
            {
                // 
                BleCharacteristic("12345679-0000-0000-0000-000000000000", onWrite, nullptr, true, true, false),
                // 
                BleCharacteristic("12345670-0000-0000-0000-000000000000", nullptr, onRead, true, true, false)
            }
        },
    });

    // Advertise the BLE device
    BleAdvertiser::advertise();
}
