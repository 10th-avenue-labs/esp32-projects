#include "SetBleConfig.h"

unique_ptr<IPlugMessageData> SetBleConfig::deserialize(const string& serialized) {
    // Create a new configuration
    SetBleConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    auto deviceName = cJSON_GetObjectItem(root.get(), "deviceName");
    config.deviceName = cJSON_IsNull(deviceName) ? nullopt : optional<string>(deviceName->valuestring);

    return make_unique<SetBleConfig>(config);
};