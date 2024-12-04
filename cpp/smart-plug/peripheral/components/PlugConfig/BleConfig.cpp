#include "BleConfig.h"

unique_ptr<cJSON, void (*)(cJSON *item)> BleConfig::serialize() {
    // Create a new cJSON object
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

    // Serialize the object
    cJSON_AddStringToObject(root.get(), "deviceName", deviceName.c_str());

    return root;
}

BleConfig BleConfig::deserialize(const string& serialized) {
    // Create a new configuration
    BleConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.deviceName = cJSON_GetObjectItem(root.get(), "deviceName")->valuestring;

    return config;
};
