#include "ble_characteristic.h"

BleCharacteristic::BleCharacteristic
(
    ble_uuid128_t *uuid,
    std::function<int(std::vector<std::byte>)> onWrite,
    bool read,
    bool write,
    bool acknowledgeWrites
) :
    uuid(uuid),
    onWrite(onWrite),
    read(read),
    write(write),
    acknowledgeWrites(acknowledgeWrites)
{
    // No impl
}
