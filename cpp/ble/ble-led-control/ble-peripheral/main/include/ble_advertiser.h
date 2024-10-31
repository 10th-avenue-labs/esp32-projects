#ifndef BLE_ADVERTISER_H
#define BLE_ADVERTISER_H

#include <string>
#include "ble_service.h"
#include <vector>
#include <map>

/* Library function declarations */
void ble_store_config_init(void); // For some reason we need to manually declare this one? Not sure why as it lives in a library

class BleAdvertiser {
public:
    const std::string deviceName;       // Device name for advertising
    uint16_t deviceAppearance;          // BLE appearance (icon or type)
    std::vector<BleService> services;   // Array of GATT services
    bool initiated;                     // Initialization status flag

    /**
     * @brief Construct a new Ble Advertiser object
     * 
     * @param deviceName The name of the device
     * @param deviceAppearance The BLE appearance
     * @param services An array of GATT services
     */
    BleAdvertiser(
        std::string deviceName,
        uint16_t deviceAppearance,
        std::vector<BleService> services
    );

    /**
     * @brief Initialize the BLE advertiser
     * 
     * @return true if successful, false otherwise
     */
    bool init();  // Initialization function

    void advertise();

private:
    std::map<uint16_t, std::function<int(std::vector<std::byte>)>> characteristicHandlesToCallbacks;

    ////////////////////////////////////////////////////////////////////////////
    // BLE helper functions
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Initialize the Generic Attribute Profile (GAP)
     * 
     * @return int 0 if successful, error code otherwise
     */
    int gapInit();

    int gattSvcInit(struct ble_gatt_svc_def *serviceDefinitions);
    void nimbleHostConfigInit(void);


    ////////////////////////////////////////////////////////////////////////////
    // Service and characteristic definition builders
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Create an array of GATT service definitions
     * 
     * @param services An array of GATT services
     * @return ble_gatt_svc_def* A pointer to the GATT service definitions
     */
    struct ble_gatt_svc_def* createServiceDefinitions(const std::vector<BleService>&);

    /**
     * @brief Create a GATT service definition
     * 
     * @param service A GATT service
     * @return ble_gatt_svc_def A GATT service definition
     */
    struct ble_gatt_svc_def createServiceDefinition(BleService);

    /**
     * @brief Create an array of GATT characteristic definitions
     * 
     * @param characteristics An array of GATT characteristics
     * @return ble_gatt_chr_def* A pointer to the GATT characteristic definitions
     */
    struct ble_gatt_chr_def* createCharacteristicDefinitions(std::vector<BleCharacteristic>);

    /**
     * @brief Create a GATT characteristic definition
     * 
     * @param characteristic A GATT characteristic
     * @return ble_gatt_chr_def A GATT characteristic definition
     */
    struct ble_gatt_chr_def createCharacteristicDefinition(BleCharacteristic characteristic);
};

static int characteristicAccessHandler(uint16_t, uint16_t, struct ble_gatt_access_ctxt*, void *);
static void onStackReset(int);
static void onStackSync(void);

#endif // BLE_ADVERTISER_HPP
