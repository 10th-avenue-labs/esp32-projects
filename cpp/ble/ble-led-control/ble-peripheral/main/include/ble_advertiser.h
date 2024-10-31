#ifndef BLE_ADVERTISER_H
#define BLE_ADVERTISER_H

#include <string>
#include "ble_service.h"


class BleAdvertiser {
public:
    std::string device_name;          // Device name for advertising
    uint16_t device_appearance;       // BLE appearance (icon or type)
    BleService* services;             // Array of GATT services
    uint8_t service_count;            // Number of services
    bool initiated;                   // Initialization status flag

    // Constructor
    BleAdvertiser(
        std::string deviceName,
        uint16_t deviceAppearance,
        BleService* services,
        uint8_t serviceCount
    );

    bool init();  // Initialization function

    void advertise();

private:
    // Private members and methods, if needed
};

#endif // BLE_ADVERTISER_HPP
