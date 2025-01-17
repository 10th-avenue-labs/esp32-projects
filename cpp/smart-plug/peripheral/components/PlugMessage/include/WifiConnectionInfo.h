#ifndef WIFI_CONNECTION_INFO_H
#define WIFI_CONNECTION_INFO_H

#include <string>
#include <cJSON.h>
#include <memory>
#include "IPlugMessageData.h"

using namespace std;

class WifiConnectionInfo : public IPlugMessageData
{
public:
    string ssid;
    string password;

    static unique_ptr<IPlugMessageData> deserialize(const string &serialized);
    string serialize() override { return ""; };
};

#endif // WIFI_CONNECTION_INFO_H