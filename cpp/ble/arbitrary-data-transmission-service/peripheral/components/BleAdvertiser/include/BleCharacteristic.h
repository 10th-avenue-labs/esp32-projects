#ifndef BLE_CHARACTERISTIC_H
#define BLE_CHARACTERISTIC_H

#include <functional>
#include <vector>
#include <string>
#include <algorithm>

extern "C"
{
    #include <host/ble_uuid.h>
    #include <esp_err.h>
    #include <string.h>
}

class BleCharacteristic {
public:
    std::function<int(std::vector<std::byte>)> onWrite;
    std::function<std::vector<std::byte>(void)> onRead;
    bool read;
    bool write;
    bool acknowledgeWrites;

    /**
     * @brief Construct a new Ble Characteristic object
     * 
     * @param uuid The UUID of the characteristic
     * @param onWrite The callback for write access
     * @param onRead The callback for read access
     * @param read Flag for whether or not to allow read access
     * @param write Flag for whether or not to allow write access
     * @param acknowledgeWrites Flag for write acknowledgment
     */
    BleCharacteristic(
        std::string uuid,
        std::function<int(std::vector<std::byte>)> onWrite,
        std::function<std::vector<std::byte>(void)> onRead,
        bool read,
        bool write,
        bool acknowledgeWrites
    );

    /**
     * @brief Destroy the Ble Characteristic object
     * 
     */
    ~BleCharacteristic();

    /**
     * @brief Get the UUID pointer
     * 
     * Note: This function exists due to the poor memory management of this BLE library. Almost everything is passed by value, so when copies of variables are made,
     * this ensures that the UUID pointer still points to the correct memory location. It would be better if we instead had a variable for the actual structure, rather than a pointer,
     * but then the structure would point to a different memory location when copied and when creating a `ble_gatt_chr_def` in the advertiser, we need a pointer to the UUID.
     * 
     * Correct memory management where everything is appropriately passed by reference and ownership is managed more appropriately would eliminate the need to use
     * a pointer type (ble_uuid_any_t*) here rather than a value type (ble_uuid_any_t*).
     * 
     * @return ble_uuid_any_t* The UUID pointer
     */
    ble_uuid_any_t* getUuidPointer();

    /**
     * @brief Convert a hex string to a uint8_t. The hex string should be 2 characters long.
     * TODO: This function should be moved to a utility class
     * 
     * @param hexStr The hex string
     * @param result The result
     * @return esp_err_t ESP_OK if successful, error code otherwise
     */
    static esp_err_t hexStringToUint8(const std::string& hexStr, uint8_t& result);

    /**
     * @brief Convert a UUID string to a ble_uuid_any_t. Currently this only supports 128-bit UUIDs. Something of the form "00000000-0000-0000-0000-000000000000"
     * TODO: This function should be moved to a utility class
     * 
     * @param uuid The UUID string
     * @param result The UUID structure to populate
     * @return esp_err_t ESP_OK if successful, error code otherwise
     */
    static esp_err_t uuidStringToUuid(std::string uuid, ble_uuid_any_t& result);
private:
    ble_uuid_any_t* uuidPointer = new ble_uuid_any_t;
    std::string uuid;
};

#endif // BLE_CHARACTERISTIC_H
