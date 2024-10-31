#ifndef BLE_CHARACTERISTIC_H
#define BLE_CHARACTERISTIC_H

// External includes
extern "C"
{
    #include <host/ble_uuid.h>
}

#include <functional>
#include <vector>

class BleCharacteristic {
public:
    ble_uuid128_t *uuid;                                    // Pointer to UUID
    std::function<int(std::vector<std::byte>)> onWrite;     // Callback for write access
    bool read;                                              // Flag for whether or not to allow read access
    bool write;                                             // Flag for whether or not to allow write access
    bool acknowledgeWrites;                                 // Flag for write acknowledgment

    /**
     * @brief Construct a new Ble Characteristic object
     * 
     * @param uuid A pointer to the UUID
     * @param onWrite A callback to execute when the characteristic is written to
     * @param read Whether or not to allow read access
     * @param write Whether or not to allow write access
     * @param acknowledgeWrites Whether or not to acknowledge writes
     */
    BleCharacteristic(
        ble_uuid128_t *uuid,
        std::function<int(std::vector<std::byte>)> onWrite,
        bool read,
        bool write,
        bool acknowledgeWrites
    );
};

#endif // BLE_CHARACTERISTIC_H
