#ifndef CLOUD_CONNECTION_INFO_H
#define CLOUD_CONNECTION_INFO_H

#include <string>
#include <cJSON.h>
#include <memory>
#include "IPlugMessageData.h"
#include "WifiConnectionInfo.h"

using namespace std;

class CloudConnectionInfo : public IPlugMessageData
{
public:
    string deviceId;
    string jwt;
    WifiConnectionInfo wifiConnectionInfo;
    string mqttConnectionString;

    static unique_ptr<IPlugMessageData> deserialize(const string &serialized);
    string serialize() override { return ""; };
};

#endif // CLOUD_CONNECTION_INFO_H