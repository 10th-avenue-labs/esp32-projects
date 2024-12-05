#include "SetWifiConfig.h"

unique_ptr<IPlugMessageData> SetWifiConfig::deserialize(const string& serialized) {
    // Create a new configuration
    SetWifiConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.ssid = cJSON_GetObjectItem(root.get(), "ssid")->valuestring;
    config.password = cJSON_GetObjectItem(root.get(), "password")->valuestring;

    return make_unique<SetWifiConfig>(config);
};