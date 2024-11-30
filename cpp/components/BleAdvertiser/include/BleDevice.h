#ifndef BLE_DEVICE_H
#define BLE_DEVICE_H

extern "C" {
    #include <cstdint>
}

using namespace std;

class BleDevice {
public:
    uint16_t connectionHandle;

    BleDevice(uint16_t connectionHandle);
private:
};

#endif // BLE_DEVICE_H
