#include "SetMqttConfig.h"

unique_ptr<IPlugMessageData> SetMqttConfig::deserialize(const string& serialized) {
    // Create a new configuration
    SetMqttConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.brokerAddress = cJSON_GetObjectItem(root.get(), "brokerAddress")->valuestring;

    return make_unique<SetMqttConfig>(config);
};