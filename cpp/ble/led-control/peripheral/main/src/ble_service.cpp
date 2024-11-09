#include "ble_service.h"

BleService::BleService(
    ble_uuid16_t *uuid,
    std::vector<BleCharacteristic> characteristics
) :
    uuid(uuid),
    characteristics(characteristics)
{
    // No impl
}