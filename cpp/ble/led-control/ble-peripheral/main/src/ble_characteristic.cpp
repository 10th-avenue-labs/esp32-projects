#include "ble_characteristic.h"

BleCharacteristic::BleCharacteristic
(
    ble_uuid128_t *uuid,
    std::function<int(std::vector<std::byte>)> onWrite,
    std::function<std::vector<std::byte>(void)> onRead,
    bool read,
    bool write,
    bool acknowledgeWrites
) :
    uuid(uuid),
    onWrite(onWrite),
    onRead(onRead),
    read(read),
    write(write),
    acknowledgeWrites(acknowledgeWrites)
{
    // No impl
}
