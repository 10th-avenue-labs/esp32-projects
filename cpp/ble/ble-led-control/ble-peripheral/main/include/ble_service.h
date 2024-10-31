#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include "ble_characteristic.h"
#include <vector>

class BleService {
public:
    ble_uuid16_t *uuid;                                 // Pointer to service UUID
    std::vector<BleCharacteristic> characteristics;     // Array of characteristics

    /**
     * @brief Construct a new Ble Service object
     * 
     * @param uuid A pointer to the service UUID
     * @param characteristics An array of characteristics
     */
    BleService(
        ble_uuid16_t *uuid,
        std::vector<BleCharacteristic> characteristics
    );
};

#endif // BLE_SERVICE_H
