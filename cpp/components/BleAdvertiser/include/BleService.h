#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include "BleCharacteristic.h"
#include <vector>
#include <string>

extern "C" {
    #include <host/ble_uuid.h>
}

class BleService {
public:
    std::vector<BleCharacteristic> characteristics;

    /**
     * @brief Construct a new Ble Service object
     * 
     * @param uuid The UUID of the service
     * @param characteristics An array of characteristics
     */
    BleService(std::string uuid, std::vector<BleCharacteristic> characteristics);

    /**
     * @brief Destroy the Ble Service object
     * 
     */
    ~BleService();

    /**
     * @brief Get the UUID pointer
     * 
     * @return ble_uuid_any_t* The UUID pointer
     */
    ble_uuid_any_t* getUuidPointer();
private:
    ble_uuid_any_t* uuidPointer = new ble_uuid_any_t;
    std::string uuid;
};

#endif // BLE_SERVICE_H
