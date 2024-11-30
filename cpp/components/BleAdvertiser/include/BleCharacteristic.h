#ifndef BLE_CHARACTERISTIC_H
#define BLE_CHARACTERISTIC_H

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <algorithm>
#include "BleDevice.h"

extern "C"
{
    #include <host/ble_uuid.h>
    #include <host/ble_gatt.h>
    #include <esp_err.h>
    #include <string.h>
    #include <host/ble_hs.h>
}

using namespace std;

class BleCharacteristic {
public:
    function<int(vector<byte>)> onWrite;
    function<vector<byte>(void)> onRead;
    function<void(shared_ptr<BleDevice>)> onSubscribe;
    bool read;
    bool write;
    bool acknowledgeWrites;

    /**
     * @brief Construct a new Ble Characteristic object
     * 
     * @param uuid The UUID of the characteristic
     * @param onWrite The callback for write access
     * @param onRead The callback for read access
     * @param acknowledgeWrites Flag for write acknowledgment
     */
    BleCharacteristic(
        string uuid,
        function<int(vector<byte>)> onWrite,
        function<vector<byte>(void)> onRead,
        function<void(shared_ptr<BleDevice>)> onSubscribe,
        bool acknowledgeWrites
    );

    // Copy constructor (from an lvalue)
    BleCharacteristic(const BleCharacteristic& other) = delete;

    // Move constructor (from an rvalue)
    BleCharacteristic(BleCharacteristic&& other);

    /**
     * @brief Destroy the Ble Characteristic object
     * 
     */
    ~BleCharacteristic();




    esp_err_t notify(vector<shared_ptr<BleDevice>> devices, vector<byte> data);



    esp_err_t populateGattCharacteristicDefinition(ble_gatt_chr_def* gattCharacteristicDefinition);


    static int characteristicAccessHandler
    (
        uint16_t conn_handle,
        uint16_t attr_handle,
        struct ble_gatt_access_ctxt *ctxt,
        void *arg
    );

    // TODO: Move to impl
    uint16_t* getHandle() {
        return characteristicHandle;
    }

    /**
     * @brief Convert a hex string to a uint8_t. The hex string should be 2 characters long.
     * TODO: This function should be moved to a utility class
     * 
     * @param hexStr The hex string
     * @param result The result
     * @return esp_err_t ESP_OK if successful, error code otherwise
     */
    static esp_err_t hexStringToUint8(const string& hexStr, uint8_t& result);

    /**
     * @brief Convert a UUID string to a ble_uuid_any_t. Currently this only supports 128-bit UUIDs. Something of the form "00000000-0000-0000-0000-000000000000"
     * TODO: This function should be moved to a utility class
     * 
     * @param uuid The UUID string
     * @param result The UUID structure to populate
     * @return esp_err_t ESP_OK if successful, error code otherwise
     */
    static esp_err_t uuidStringToUuid(string uuid, ble_uuid_any_t& result);
private:
    ble_uuid_any_t uuidDefinition;
    uint16_t* characteristicHandle = new uint16_t(0);
};

#endif // BLE_CHARACTERISTIC_H
