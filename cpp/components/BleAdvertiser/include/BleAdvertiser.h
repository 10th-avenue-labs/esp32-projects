#ifndef BLE_ADVERTISER_H
#define BLE_ADVERTISER_H

#include <string>
#include "BleService.h"
#include "BleCharacteristic.h"
#include <vector>
#include <map>

extern "C"
{
    #include <esp_log.h>
    #include <esp_err.h>
    #include <nvs_flash.h>
    #include <nimble/nimble_port.h>
    #include <services/gap/ble_svc_gap.h>
    #include <host/ble_gatt.h>
    #include <services/gatt/ble_svc_gatt.h>
    #include <host/ble_hs.h>
    #include <host/util/util.h>
    #include "nimble/ble.h"

    /* Library function declarations */
    void ble_store_config_init(void); // For some reason we need to manually declare this one? Not sure why as it lives in a library
}

class BleAdvertiser {
    /**
     * Generally, the way the BLE advertiser works is as follows:
     *  1. Initialize the BLE advertiser with a
     *      a. device name,
     *      b. appearance,
     *      c. role,
     *      d. and an array of GATT services
     *  2. Services are defined by a UUID and an array of characteristics
     *      a. Characteristics are defined by a UUID, read/write callbacks, and read/write permissions
     *  2. Advertise the BLE device
     * 
     */

public:
    /**
     * @brief Initialize the BLE advertiser
     * 
     * @param deviceName The name of the device to advertise
     * @param deviceAppearance The BLE appearance
     * @param deviceRole The BLE role
     * @param services An array of GATT services
     * @return true if successful, false otherwise
     */
    static bool init(
        std::string deviceName,
        uint16_t deviceAppearance,
        uint8_t deviceRole,
        std::vector<std::shared_ptr<BleService>>&& services
    );

    // TODO: Constructor with rvalue args

    /**
     * @brief Advertise the BLE device. This function will not return until the BLE stack is stopped
     * 
     */
    static void advertise(void);

    static void shutdown(void);

    /**
     * @brief Get the MTU
     * 
     * @return uint16_t The MTU
     */
    static uint16_t getMtu(void);
private:
    // Private constructor to prevent instantiation
    BleAdvertiser() = delete;

    static std::string deviceName;
    static uint16_t deviceAppearance;
    static uint8_t deviceRole;
    static bool initiated;
    static uint8_t deviceAddressType;
    static uint8_t deviceAddress[6];
    static uint16_t mtu;
    static ble_gatt_svc_def* gattServiceDefinitions;
    static std::vector<std::shared_ptr<BleService>> services;

    ////////////////////////////////////////////////////////////////////////////
    // BLE helper functions
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Format a BLE address as a string
     * 
     * @param addressString The formatted address string
     * @param address The BLE address
     */
    static inline void formatAddress(char *addressString, uint8_t address[]);

    /**
     * @brief Print a BLE connection description
     * 
     * @param connectionDescription The BLE connection description
     */
    static void printConnectionDescription(struct ble_gap_conn_desc *connectionDescription);

    /**
     * @brief Initialize the Generic Attribute Profile (GAP)
     * 
     * @return int 0 if successful, error code otherwise
     */
    static int gapInit(void);

    /**
     * @brief Initialize the GATT server
     * 
     *  1. Initialize GATT service
     *  2. Update NimBLE host GATT services counter
     *  3. Add GATT services to server
     * 
     * @param serviceDefinitions An array of GATT service definitions
     * @return int 0 if successful, error code otherwise
     */
    static int gattSvcInit(struct ble_gatt_svc_def *serviceDefinitions);

    /**
     * @brief Initialize the BLE host configuration
     * 
     */
    static void nimbleHostConfigInit(void);

    /**
     * @brief Handle GAP events
     * 
     * @param event The GAP event
     * @param arg The argument
     * @return int 0 if successful, error code otherwise
     */
    static int gapEventHandler(struct ble_gap_event*, void*);

    /**
     * @brief Handle MTU events
     * 
     * @param conn_handle The connection handle
     * @param error The GATT error
     * @param mtu The MTU
     * @param arg The argument
     * @return int 0 if successful, error code otherwise
     */
    static int mtuEventHandler(uint16_t conn_handle, const ble_gatt_error *error, uint16_t mtu, void *arg);

    ////////////////////////////////////////////////////////////////////////////
    // Nimble stack event callback functions
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Callback function for when the stack resets, called when host resets BLE stack due to error
     * 
     * @param reason The reason for the reset
     */
    static void onStackReset(int);

    /**
     * @brief Callback function for when the stack syncs with the controller, called when host has synced with controller
     * 
     */
    static void onStackSync(void);

    /**
     * @brief Handle GATT attribute register events
     * 
     *  - Service register event
     *  - Characteristic register event
     *  - Descriptor register event
     * 
     * @param ctxt The GATT register context
     * @param arg The argument
     */
    static void gattSvrRegisterCb(struct ble_gatt_register_ctxt*, void*);

    ////////////////////////////////////////////////////////////////////////////
    // Advertising helper functions
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Initialize advertising
     * 
     */
    static void initializeAdvertising(void);

    /**
     * @brief Start advertising
     * 
     * @param services An array of GATT services
     * @return ble_gatt_svc_def* A pointer to the GATT service definitions
     */
    static void startAdvertising(void);

    ////////////////////////////////////////////////////////////////////////////
    // Service definition builder
    ////////////////////////////////////////////////////////////////////////////

    static esp_err_t createGattServiceDefinitions();
};

#endif // BLE_ADVERTISER_HPP
