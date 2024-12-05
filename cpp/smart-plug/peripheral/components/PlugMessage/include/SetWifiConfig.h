#ifndef SET_WIFI_CONFIG_H
#define SET_WIFI_CONFIG_H

#include <string>
#include <cJSON.h>
#include <memory>
#include "IPlugMessageData.h"

using namespace std;

class SetWifiConfig: public IPlugMessageData {
    public:
        string ssid;
        string password;

        static unique_ptr<IPlugMessageData> deserialize(const string& serialized);
};

#endif // SET_WIFI_CONFIG_H