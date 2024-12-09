#include "BleCharacteristic.h"
#include "BleAdvertiser.h"

static const char* TAG = "BLE_CHARACTERISTIC";

////////////////////////////////////////////////////////////////////////////////
/// Constructors / Destructors
////////////////////////////////////////////////////////////////////////////////

BleCharacteristic::BleCharacteristic(
    string uuid,
    function<int(vector<byte>, shared_ptr<BleDevice>)> onWrite,
    function<vector<byte>(void)> onRead,
    function<void(shared_ptr<BleDevice>)> onSubscribe,
    bool acknowledgeWrites
):
    onWrite(onWrite),
    onRead(onRead),
    onSubscribe(onSubscribe),
    acknowledgeWrites(acknowledgeWrites)
{
    // Set the read and write flags
    read = onRead != nullptr;
    write = onWrite != nullptr;

    // Populate the UUID structure from the UUID string
    ESP_ERROR_CHECK(uuidStringToUuid(uuid, this->uuidDefinition));
};


BleCharacteristic::BleCharacteristic(BleCharacteristic&& other) {
    this->uuidDefinition = move(other.uuidDefinition);
    this->onWrite = move(other.onWrite);
    this->onRead = move(other.onRead);
    this->onSubscribe = move(other.onSubscribe);
    this->acknowledgeWrites = move(other.acknowledgeWrites);
    this->read = move(other.read);
    this->write = move(other.write);
}

BleCharacteristic::~BleCharacteristic()
{
    delete characteristicHandle;
}

////////////////////////////////////////////////////////////////////////////////
/// Public functions
////////////////////////////////////////////////////////////////////////////////

esp_err_t BleCharacteristic::notify(vector<shared_ptr<BleDevice>> devices, vector<byte> data) {
    // Create a new mbuf
    os_mbuf* om = os_msys_get_pkthdr(data.size(), 0);
    if (om == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    // Copy the data to the nimble stack
    int response = os_mbuf_append(om, data.data(), data.size());
    if (response != 0) {
        return ESP_FAIL;
    }

    // Iterate through devices, sending a notification to each
    for(auto device : devices) {
        ble_gatts_notify_custom(
            device->connectionHandle,
            *characteristicHandle,
            om
        );
    }

    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////
/// Friend functions
////////////////////////////////////////////////////////////////////////////////

esp_err_t BleCharacteristic::populateGattCharacteristicDefinition(ble_gatt_chr_def* gattCharacteristicDefinition) {
    // Populate the flags
    ble_gatt_chr_flags flags = 0;
    flags = flags | (read ? BLE_GATT_CHR_F_READ : 0);
    ble_gatt_chr_flags acknowledgeWritesFlag = acknowledgeWrites ? BLE_GATT_CHR_F_WRITE : BLE_GATT_CHR_F_WRITE_NO_RSP;
    flags = write ? (flags | acknowledgeWritesFlag) : flags;
    // TODO: Add support for notify and indicate. This should be passed in from the constructor
    flags = flags | BLE_GATT_CHR_F_NOTIFY;

    *gattCharacteristicDefinition = (struct ble_gatt_chr_def)
    {
        .uuid = &this->uuidDefinition.u,
        .access_cb = characteristicAccessHandler,
        .arg = this,
        .flags = flags,
        .val_handle = characteristicHandle,
    };

    return ESP_OK;
}

uint16_t* BleCharacteristic::getHandle() {
    return characteristicHandle;
}

////////////////////////////////////////////////////////////////////////////////
/// Private functions
////////////////////////////////////////////////////////////////////////////////

int BleCharacteristic::characteristicAccessHandler
(
    uint16_t connectionHandle,
    uint16_t attributeHandle,
    struct ble_gatt_access_ctxt *gattAccessContext,
    void *arg
) {
    // Get the characteristic from the argument
    BleCharacteristic* characteristic = (BleCharacteristic*) arg;

    // Handle access events
    switch (gattAccessContext->op) {
        // Read characteristic
        case BLE_GATT_ACCESS_OP_READ_CHR: {
            // Verify the connection handle
            if (connectionHandle != BLE_HS_CONN_HANDLE_NONE) {
                // The characteristic was read by a connected device
                ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                        connectionHandle, attributeHandle);
            } else {
                // The characteristic was read by the nimble stack
                ESP_LOGI(TAG,
                        "characteristic read by nimble stack; attr_handle=%d",
                        attributeHandle);
            }

            // Check if the characteristic handle is the same as the attribute handle
            if (*characteristic->characteristicHandle != attributeHandle) {
                ESP_LOGE(TAG, "unknown attribute handle: %d", attributeHandle);
                return BLE_ATT_ERR_UNLIKELY;
            }

            // Check if the characteristic has a read callback
            if (characteristic->onRead == nullptr) {
                ESP_LOGE(TAG, "characteristic does not have a read callback");
                return BLE_ATT_ERR_UNLIKELY;
            }

            // Get the data from the characteristic
            vector<byte> data = characteristic->onRead();

            // Copy the data to the nimble stack
            int response = os_mbuf_append(gattAccessContext->om, data.data(), data.size());

            // Return the response
            return response == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        // Write characteristic
        case BLE_GATT_ACCESS_OP_WRITE_CHR: {
            // Verify the connection handle
            if (connectionHandle != BLE_HS_CONN_HANDLE_NONE) {
                // The characteristic was written to by a connected device
                ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                        connectionHandle, attributeHandle);
            } else {
                // The characteristic was written to by the nimble stack
                ESP_LOGI(TAG,
                        "characteristic write by nimble stack; attr_handle=%d",
                        attributeHandle);
            }

            // Check if the characteristic handle is the same as the attribute handle
            if (*characteristic->characteristicHandle != attributeHandle) {
                ESP_LOGE(TAG, "unknown attribute handle: %d", attributeHandle);
                return BLE_ATT_ERR_UNLIKELY;
            }

            // Check if the characteristic has a write callback
            if (characteristic->onWrite == nullptr) {
                ESP_LOGE(TAG, "characteristic does not have a write callback");
                return BLE_ATT_ERR_UNLIKELY;
            }

            // Get the device that wrote to the characteristic
            shared_ptr<BleDevice> device = BleAdvertiser::connectedDevicesByHandle[connectionHandle];
            if (device == nullptr) {
                ESP_LOGE(TAG, "device not found for connection handle: %d", connectionHandle);
                return BLE_ATT_ERR_UNLIKELY;
            }

            // Convert the data to a vector of bytes
            vector<byte> data = vector<byte>(
                (byte*) gattAccessContext->om->om_data,
                (byte*) gattAccessContext->om->om_data + gattAccessContext->om->om_len
            );

            // Call the write callback
            return characteristic->onWrite(data, device);
        }
        // Read descriptor
        case BLE_GATT_ACCESS_OP_READ_DSC:
            ESP_LOGE(TAG, "operation not implemented, opcode: %d", gattAccessContext->op);
            return BLE_ATT_ERR_UNLIKELY;
        // Write descriptor
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            ESP_LOGE(TAG, "operation not implemented, opcode: %d", gattAccessContext->op);
            return BLE_ATT_ERR_UNLIKELY;
        // Unknown event
        default:
            ESP_LOGE(TAG,
                "unexpected access operation to led characteristic, opcode: %d",
                gattAccessContext->op);
            return BLE_ATT_ERR_UNLIKELY;
    }

    // Control shouldn't reach here
}

////////////////////////////////////////////////////////////////////////////////
/// Helper functions (TODO: These should be moved to a utility class)
////////////////////////////////////////////////////////////////////////////////

esp_err_t BleCharacteristic::hexStringToUint8(const string& hexStr, uint8_t& result) {
    if (hexStr.size() != 2) {
        return ESP_ERR_INVALID_ARG;
    }

    // Convert the string to an integer using stoi with base 16
    int value = stoi(hexStr, nullptr, 16);

    // Ensure the value fits in a uint8_t
    if (value < 0 || value > 255) {
        return ESP_ERR_INVALID_ARG;
    }

    // Set the result and return
    result = static_cast<uint8_t>(value);
    return ESP_OK;
};

esp_err_t BleCharacteristic::uuidStringToUuid(string uuid, ble_uuid_any_t& result) {
    // Remove all dashes from string
    uuid.erase(remove(uuid.begin(), uuid.end(), '-'), uuid.end());

    switch (uuid.size()) {
        // 128-bit UUID
        case 32:
            {
                // Set the type of the result (are both of these necessary?)
                result.u.type = BLE_UUID_TYPE_128;
                result.u128.u.type = BLE_UUID_TYPE_128;

                // uint8_t uuidBytes[16];
                for (size_t i = 0; i < uuid.size(); i += 2) {
                    string hexString = uuid.substr(i, 2);

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