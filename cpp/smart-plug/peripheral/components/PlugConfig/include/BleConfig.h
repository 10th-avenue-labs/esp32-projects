#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include <memory>
#include "cJSON.h"

using namespace std;

class BleConfig {
    public:
        string deviceName;

    unique_ptr<cJSON, void (*)(cJSON *item)> serialize();
    static BleConfig deserialize(const string& serialized);

};

#endif // BLE_CONFIG_H