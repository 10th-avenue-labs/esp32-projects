#ifndef SET_BLE_CONFIG_H
#define SET_BLE_CONFIG_H

#include <string>
#include <cJSON.h>
#include <memory>
#include "IPlugMessageData.h"

using namespace std;

class SetBleConfig : public IPlugMessageData {
    public:
        optional<string> deviceName;

        static unique_ptr<IPlugMessageData> deserialize(const string& serialized);
        string serialize() override { return ""; };
};

#endif // SET_BLE_CONFIG_H