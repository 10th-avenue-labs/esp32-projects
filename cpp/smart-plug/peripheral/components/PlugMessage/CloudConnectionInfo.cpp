#include "CloudConnectionInfo.h"

unique_ptr<IPlugMessageData> CloudConnectionInfo::deserialize(const string &serialized)
{
    // Create a new configuration
    CloudConnectionInfo config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.deviceId = cJSON_GetObjectItem(root.get(), "deviceId")->valuestring;
    config.jwt = cJSON_GetObjectItem(root.get(), "jwt")->valuestring;
    config.wifiConnectionInfo = *dynamic_cast<WifiConnectionInfo *>(WifiConnectionInfo::deserialize(cJSON_Print(cJSON_GetObjectItem(root.get(), "wifiConnectionInfo"))).get());
    config.mqttConnectionString = cJSON_GetObjectItem(root.get(), "mqttConnectionString")->valuestring;

    return make_unique<CloudConnectionInfo>(config);
};