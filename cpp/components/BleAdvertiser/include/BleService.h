#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <memory>
#include "BleCharacteristic.h"
#include <vector>
#include <string>

extern "C" {
    #include <host/ble_uuid.h>
    #include <host/ble_gatt.h>
    #include <esp_err.h>
    #include <esp_log.h>
}

using namespace std;

class BleService {
public:
    vector<shared_ptr<BleCharacteristic>> characteristics;

    ////////////////////////////////////////////////////////////////////////////
    /// Constructors / Destructors
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Construct a new Ble Service object
     * 
     * @param uuid The UUID of the service
     * @param characteristics An array of rvalue characteristics
     */
    BleService(string uuid, vector<BleCharacteristic>&& characteristics);

    /**
     * @brief Construct a new Ble Service object
     * 
     * @param uuid The UUID of the service
     * @param characteristics An array of shared pointer characteristics
     */
    BleService(string uuid, vector<shared_ptr<BleCharacteristic>> characteristics);

    /**
     * @brief Disallow copy constructor (from an lvalue)
     * 
     * @param other The other object
     */
    BleService(const BleService& other) = delete;

    /**
     * @brief Move constructor (from an rvalue)
     * 
     * @param other The other object
     */
    BleService(BleService&& other);

    /**
     * @brief Destroy the Ble Service object
     * 
     */
    ~BleService();

    ////////////////////////////////////////////////////////////////////////////
    /// Friend functions
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Populate the GATT service definition
     * 
     * @param gattServiceDefinition The memory location of the GATT service definition
     * @return esp_err_t ESP_OK if successful, error code otherwise
     */
    esp_err_t populateGattServiceDefinition(ble_gatt_svc_def* gattServiceDefinition);
private:
    ble_uuid_any_t uuidDefinition;
    ble_gatt_chr_def* gattCharacteristicDefinitions;

    /**
     * @brief Create the GATT service definition
     * 
     * @return esp_err_t ESP_OK if successful, error code otherwise
     */
    esp_err_t createGattCharacteristicDefinitions();
};

#endif // BLE_SERVICE_H
