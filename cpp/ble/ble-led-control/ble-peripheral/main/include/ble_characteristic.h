#ifndef BLE_CHARACTERISTIC_H
#define BLE_CHARACTERISTIC_H

// External includes
extern "C"
{
    #include <host/ble_uuid.h>
    #include <host/ble_gatt.h>
}

#include <functional>

using onWrite = std::function<int(std::vector<std::byte>)>;

class BleCharacteristic {
public:
    ble_uuid128_t *uuid;                        // Pointer to UUID
    onWrite on_write;                           // Callback for write access
    bool read;                                  // Flag for whether or not to allow read access
    bool write;                                 // Flag for whether or not to allow write access
    bool acknowledge_writes;                    // Flag for write acknowledgment

    BleCharacteristic(
        ble_uuid128_t *uuid,
        bool read,
        bool write,
        bool acknowledge_writes,
        onWrite on_write
    );
};

#endif // BLE_CHARACTERISTIC_H
