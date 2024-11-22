#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include "ble_characteristic.h"
#include <vector>

extern "C" {
    #include <host/ble_uuid.h>
}

class BleService {
public:
    ble_uuid16_t *uuid;                                 // Pointer to service UUID
    std::vector<BleCharacteristic> characteristics;     // Array of characteristics
};

#endif // BLE_SERVICE_H
