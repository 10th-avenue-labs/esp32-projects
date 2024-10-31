#include "ble_characteristic.h"

// Constructor
BleCharacteristic::BleCharacteristic
(
    ble_uuid128_t *uuid,
    bool read,
    bool write,
    bool acknowledge_writes,
    onWrite on_write
) :
    uuid(uuid), 
    read(read), 
    write(write), 
    acknowledge_writes(acknowledge_writes), 
    on_write(on_write)
{
    // No impl
}
