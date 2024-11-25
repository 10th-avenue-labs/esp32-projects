#include "BleCharacteristic.h"

BleCharacteristic::BleCharacteristic(
    std::string uuid,
    std::function<int(std::vector<std::byte>)> onWrite,
    std::function<std::vector<std::byte>(void)> onRead,
    bool read,
    bool write,
    bool acknowledgeWrites)
{
    this->uuid = uuid;
    this->onWrite = onWrite;
    this->onRead = onRead;
    this->read = read;
    this->write = write;
    this->acknowledgeWrites = acknowledgeWrites;

    // Populate the UUID structure from the UUID string
    ESP_ERROR_CHECK(uuidStringToUuid(uuid, *uuidPointer));
};

BleCharacteristic::~BleCharacteristic()
{
    // FIXME: Implement memory management, currently, this is a memory leak
    // delete uuidPointer;
}

ble_uuid_any_t* BleCharacteristic::getUuidPointer() {
    return uuidPointer;
}

esp_err_t BleCharacteristic::hexStringToUint8(const std::string& hexStr, uint8_t& result) {
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
};

esp_err_t BleCharacteristic::uuidStringToUuid(std::string uuid, ble_uuid_any_t& result) {
    // Remove all dashes from string
    uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end());

    switch (uuid.size()) {
        // 128-bit UUID
        case 32:
            {
                // Set the type of the result (are both of these necessary?)
                result.u.type = BLE_UUID_TYPE_128;
                result.u128.u.type = BLE_UUID_TYPE_128;

                // uint8_t uuidBytes[16];
                for (size_t i = 0; i < uuid.size(); i += 2) {
                    std::string hexString = uuid.substr(i, 2);

                    uint8_t val;
                    ESP_ERROR_CHECK(hexStringToUint8(hexString, val));
                    
                    // Populate the UUID bytes in reverse order
                    result.u128.value[15 - (i / 2)] = val;
                }

                return ESP_OK;
            }
        case 8:
            // TODO: Implement 32-bit UUID (these are of the hexidecimal form 00000000)
            break;
        case 4:
            // TODO: Implement 16-bit UUID (these are of the hexidecimal form 0000)
            break;
        default:
            // TODO: Implement error handling
            break;
    }

    // return ble_uuid_any_t();
    return ESP_FAIL;
}