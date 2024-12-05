#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <memory>
#include "cJSON.h"

using namespace std;

class WifiConfig {
    public:
        string ssid;
        string password;

        unique_ptr<cJSON, void (*)(cJSON *item)> serialize();
        static WifiConfig deserialize(const string& serialized);
};

#endif // WIFI_CONFIG_H