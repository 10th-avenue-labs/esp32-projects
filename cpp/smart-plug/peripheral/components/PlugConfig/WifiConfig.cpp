#include "WifiConfig.h"

unique_ptr<cJSON, void (*)(cJSON *item)> WifiConfig::serialize() {
    // Create a new cJSON object
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

    // Serialize the object
    cJSON_AddStringToObject(root.get(), "ssid", ssid.c_str());
    cJSON_AddStringToObject(root.get(), "password", password.c_str());

    return root;
}

WifiConfig WifiConfig::deserialize(const string& serialized) {
    // Create a new configuration
    WifiConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.ssid = cJSON_GetObjectItem(root.get(), "ssid")->valuestring;
    config.password = cJSON_GetObjectItem(root.get(), "password")->valuestring;

    return config;
};
