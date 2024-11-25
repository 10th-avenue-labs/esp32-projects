#include <algorithm>
#include <string>
#include "BleAdvertiser.h"
#include "BleService.h"

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

extern "C" void app_main(void)
{
    // Create a BLE characteristic
    BleCharacteristic dimmerBrightnessCharacteristic(
        "12345678-0000-0000-0000-000000000000",
        nullptr,
        nullptr,
        true,
        true,
        false
    );

    // Create a BLE service
    BleService dimmerService("00000000-0000-0000-0000-000000012345", {dimmerBrightnessCharacteristic});

    // Initialize the BLE advertiser
    BleAdvertiser::init("Dimmer", BLE_GAP_APPEARANCE_GENERIC_TAG, BLE_GAP_LE_ROLE_PERIPHERAL, {dimmerService});

    // Advertise the BLE device
    BleAdvertiser::advertise();
}
