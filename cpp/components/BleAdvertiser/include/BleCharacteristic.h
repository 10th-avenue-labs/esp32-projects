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

/**
 * In order to make everything truly as robust and efficient as possible, while also being remarkable easy to use. We might consider a refactor.
 * 
 * In the refactor, we would create a shared data class or struct `BleCharacteristicData` that would hold ALL the characteristics data.
 *  - Some data in the BleCharacteristicData NEEDS to be shared (uuidDefinition, characteristicHandle, and callbacks). This data MUST be shared because on BLE init,
 *    we reference specific memory locations to init. These same memory location are then used again later in the BLE stack. If we were to move or destroy
 *    the data after this point, we would have a dangling pointer, thus needing the data to be shared. Right now we "share" the data by preventing this class
 *    from ever being copied. Right now we allow move, but this can probably lead to bugs in some scenarios. We only allow move right now because we have to
 *    in order for things to compile with how we're currently using the class. It isn't actually moved at runtime from tests.
 *  - Other data id the class (flags) doesn't NEED to be shared, but it also doesn't make any sense for the data to be duplicated. This data is all essentially
 *    either computed or const on initialization. We could share it strictly for memory efficiency and logical design (not giving the user enough rope to get confused).
 * The BleCharacteristic class would then be a wrapper around the BleCharacteristicData class with smart pointers keeping track of the data. This would make it so that
 * users of this class could essentially do whatever they want, copy, move, assign, etc without any consequences. Copying the class would simply increment
 * the shared pointers usage.
 * 
 * Pros:
 *  - The class would be more robust/more understandable/and more permissive in it's use.
 * Cons:
 *  - The current implementation forces the user to use shared pointers themselves. This way they know that the class con not be copied or moved.
 *     - Is this really a good thing? This would make writing helper functions quite difficult. It seems that situations could arise where it would be useful
 *       to copy the class, like a helper function to perhaps print class information. With the current implementation this would not be possible.
 * 
 * In either case, it would be a good idea for BleService and BleDevice to be refactored in the same way with BleAdvertiser taking as many similarities as it can.
 * 
 * Conclusion: The refactor would be a good idea, but we're going to table this for now.
 */

class BleCharacteristic {
public:
    function<int(vector<byte>)> onWrite;
    function<vector<byte>(void)> onRead;
    function<void(shared_ptr<BleDevice>)> onSubscribe;
    bool read;
    bool write;
    bool acknowledgeWrites;

    ////////////////////////////////////////////////////////////////////////////
    /// Constructors / Destructors
    ////////////////////////////////////////////////////////////////////////////

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

    /**
     * @brief Disallow copy constructor (from an lvalue)
     * 
     * @param other The other object
     */
    BleCharacteristic(const BleCharacteristic& other) = delete;

    /**
     * @brief Move constructor (from an rvalue)
     * 
     * @param other The other object
     */
    BleCharacteristic(BleCharacteristic&& other);

    /**
     * @brief Destroy the Ble Characteristic object
     * 
     */
    ~BleCharacteristic();

    ////////////////////////////////////////////////////////////////////////////
    /// Public functions
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Notify devices of a change in the characteristic
     * 
     * @param devices The devices to notify
     * @param data The data to send
     * @return esp_err_t ESP_OK if successful, error code otherwise
     */
    esp_err_t notify(vector<shared_ptr<BleDevice>> devices, vector<byte> data);

    ////////////////////////////////////////////////////////////////////////////
    /// Friend functions
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Populate the GATT characteristic definition
     * 
     * @param gattCharacteristicDefinition The memory location of the GATT characteristic definition
     * @return esp_err_t ESP_OK if successful, error code otherwise
     */
    esp_err_t populateGattCharacteristicDefinition(ble_gatt_chr_def* gattCharacteristicDefinition);

    /**
     * @brief Get the handle of the characteristic
     * 
     * @return uint16_t* The handle
     */
    uint16_t* getHandle();

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

    /**
     * @brief Handle characteristic access events
     * 
     * @param connectionHandle The connection handle of the device associated with the event
     * @param attributeHandle The attribute handle of the characteristic associated with the event
     * @param gattAccessContext The GATT access context
     * @param arg User-specified arguments, in this case a reference to the characteristic
     * @return int 0 if successful, error code otherwise
     */
    static int characteristicAccessHandler
    (
        uint16_t connectionHandle,
        uint16_t attributeHandle,
        struct ble_gatt_access_ctxt *gattAccessContext,
        void *arg
    );
};

#endif // BLE_CHARACTERISTIC_H
