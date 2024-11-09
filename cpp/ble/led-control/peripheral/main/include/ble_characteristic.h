#ifndef BLE_CHARACTERISTIC_H
#define BLE_CHARACTERISTIC_H

#include <functional>
#include <vector>

extern "C"
{
    #include <host/ble_uuid.h>
}

class BleCharacteristic {
public:
    ble_uuid128_t *uuid;                                    // Pointer to UUID. TODO: Figure out how to simply pass the uuid in as a string
    std::function<int(std::vector<std::byte>)> onWrite;     // Callback for write access
    std::function<std::vector<std::byte>(void)> onRead;     // Callback for read access
    bool read;                                              // Flag for whether or not to allow read access
    bool write;                                             // Flag for whether or not to allow write access
    bool acknowledgeWrites;                                 // Flag for write acknowledgment

    /**
     * @brief Construct a new Ble Characteristic object
     * 
     * @param uuid A pointer to the UUID
     * @param onWrite A callback to execute when the characteristic is written to
     * @param onRead A callback to execute when the characteristic is read
     * @param read Whether or not to allow read access
     * @param write Whether or not to allow write access
     * @param acknowledgeWrites Whether or not to acknowledge writes
     */
    BleCharacteristic(
        ble_uuid128_t *uuid,
        std::function<int(std::vector<std::byte>)> onWrite,
        std::function<std::vector<std::byte>(void)> onRead,
        bool read,
        bool write,
        bool acknowledgeWrites
    );
};

#endif // BLE_CHARACTERISTIC_H
