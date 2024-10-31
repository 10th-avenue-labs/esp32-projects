#include "ble_advertiser.h"

// Constructor
BleAdvertiser::BleAdvertiser
(
    std::string deviceName,
    uint16_t deviceAppearance,
    BleService* services,
    uint8_t serviceCount
):
    device_name(std::move(deviceName)), 
    device_appearance(deviceAppearance), 
    services(services), 
    service_count(serviceCount), 
    initiated(false)
{
    // No impl
}

bool BleAdvertiser::init() {
    return false;
}
