#include "SetAcDimmerConfig.h"

unique_ptr<IPlugMessageData> SetAcDimmerConfig::deserialize(const string& serialized) {
    // Create a new configuration
    SetAcDimmerConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.brightness = cJSON_GetObjectItem(root.get(), "brightness")->valueint;

    return make_unique<SetAcDimmerConfig>(config);
};