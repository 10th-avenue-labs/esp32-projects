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
};

#endif // BLE_CHARACTERISTIC_H
