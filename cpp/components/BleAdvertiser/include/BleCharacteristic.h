#ifndef BLE_CHARACTERISTIC_H
#define BLE_CHARACTERISTIC_H

#include <functional>
#include <vector>
#include <string>
#include <algorithm>

extern "C"
{
    #include <host/ble_uuid.h>
    #include <host/ble_gatt.h>
    #include <esp_err.h>
    #include <string.h>
    #include <host/ble_hs.h>
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



    esp_err_t populateGattCharacteristicDefinition(ble_gatt_chr_def* gattCharacteristicDefinition);


    static int BleCharacteristic::characteristicAccessHandler
    (
        uint16_t conn_handle,
        uint16_t attr_handle,
        struct ble_gatt_access_ctxt *ctxt,
        void *arg
    );




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
    ble_uuid_any_t uuidDefinition;
    uint16_t* characteristicHandle = new uint16_t(0);
};

#endif // BLE_CHARACTERISTIC_H
