#include <algorithm>
#include <string>
#include "BleAdvertiser.h"
// #include "BleService.h"

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


// Dimmer service
static ble_uuid16_t dimmerServiceUuid = BLE_UUID16_INIT(0x3246);
static ble_uuid128_t dimmerBrightnessUuid =
    BLE_UUID128_INIT(0x32, 0x46, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

static std::string arbitrartDataTransferServiceUuid = "00000000-0000-0000-0000-000000000000";
//                                                       "ad56e0ff-56d4-4b73-8bca-d4f37c1c3364"
//                                                       "00001525-1212-efde-1523-785feabc4532




uint8_t hexStringToUint8(const std::string& hexStr, uint8_t& result) {
    if (hexStr.size() != 2) {
        return ESP_ERR_INVALID_ARG;
    }

    // Convert the string to an integer using std::stoi with base 16
    int value = std::stoi(hexStr, nullptr, 16);

    // Ensure the value fits in a uint8_t
    if (value < 0 || value > 255) {
        return ESP_ERR_INVALID_ARG;
    }

    // Set the result and return
    result = static_cast<uint8_t>(value);
    return ESP_OK;
}

static ble_uuid_any_t uuidStringToUuid(std::string uuid) {
    // Remove all dashes from string
    uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end());

// 128, 32, 00000000000000000000000000000000, 16
// 32, 8, 00000000
// 16, 4, 0000

    switch (uuid.size()) {
        // 128-bit UUID
        case 32:
            {
                uint8_t uuidBytes[16];
                for (size_t i = 0; i < uuid.size(); i += 2) {
                    std::string hexString = uuid.substr(i, 2);

                    uint8_t val;
                    ESP_ERROR_CHECK(hexStringToUint8(hexString, val));
                    uuidBytes[15 - (i / 2)] = val;
                }

                ble_uuid128_t bleUuid128;
                bleUuid128.u.type = BLE_UUID_TYPE_128;
                memcpy(bleUuid128.value, uuidBytes, 16);

                ble_uuid_any_t bleUuidAny;
                bleUuidAny.u128 = bleUuid128;
                return bleUuidAny;
            }
        case 8:
            break;
        case 4:
            break;
        default:
            // Error
            break;
    }

    return ble_uuid_any_t();
}

extern "C" void app_main(void)
{
    // std::string test = "01234567-89ab-0000-0000-000000000000";
    std::string test = "00000000-0000-1234-5678-9abcdef00000";
    std::string uiid = "00000000-0000-0000-0000-000000012345";
    ble_uuid_any_t uuid = uuidStringToUuid(uiid);
    for(int i = 0; i < 16; i++) {
        printf("val: %02x\n", uuid.u128.value[i]);
    }

    // Create a BLE characteristic
    BleCharacteristic dimmerBrightnessCharacteristic;
    dimmerBrightnessCharacteristic.uuid = &uuid.u128;

    // Create a BLE service
    BleService dimmerService(&dimmerServiceUuid, {dimmerBrightnessCharacteristic});

    // Initialize the BLE advertiser
    BleAdvertiser::init("Dimmer", BLE_GAP_APPEARANCE_GENERIC_TAG, BLE_GAP_LE_ROLE_PERIPHERAL, {dimmerService});

    // Advertise the BLE device
    BleAdvertiser::advertise();
}


// Two ID types
// 128-bit UUIDs
// 16-bit UUIDs

// 128bit: 00000000-0000-0000-0000–000000000000
// 16bit:  0000XXXX-0000-0000-0000–000000000000
// 16bit just have the corresponding XXXX values changed within the ID

