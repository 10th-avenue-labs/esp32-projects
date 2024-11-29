#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include "BleCharacteristic.h"
#include <vector>
#include <string>
#include <memory>

extern "C" {
    #include <host/ble_uuid.h>
    #include <esp_err.h>
    #include <esp_log.h>
}

using namespace std;

class BleService {
public:
    vector<shared_ptr<BleCharacteristic>> characteristics;

    /**
     * @brief Construct a new Ble Service object
     * 
     * @param uuid The UUID of the service
     * @param characteristics An array of characteristics
     */
    BleService(string uuid, vector<BleCharacteristic>&& characteristics);

    /**
     * @brief Construct a new Ble Service object
     * 
     * @param uuid The UUID of the service
     * @param characteristics An array of characteristics
     */
    BleService(string uuid, vector<shared_ptr<BleCharacteristic>> characteristics);

    /**
     * @brief Destroy the Ble Service object
     * 
     */
    ~BleService();


    // ble_gatt_svc_def* getGattServiceDefinition();


    esp_err_t createGattCharacteristicDefinitions();

    // Populate the gatt service definition into a specified piece of memory
    esp_err_t populateGattServiceDefinition(ble_gatt_svc_def* gattServiceDefinition);

private:
    ble_uuid_any_t uuidDefinition;
    ble_gatt_chr_def* gattCharacteristicDefinitions;
};

#endif // BLE_SERVICE_H
