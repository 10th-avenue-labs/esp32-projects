#ifndef BLE_DEVICE_H
#define BLE_DEVICE_H

extern "C" {
    #include <cstdint>
}

using namespace std;

class BleDevice {
public:
    uint16_t connectionHandle;

    /**
     * @brief Construct a new Ble Device object
     * 
     * @param connectionHandle The connection handle of the device
     */
    BleDevice(uint16_t connectionHandle);
};

#endif // BLE_DEVICE_H
