#include "MqttConfig.h"

unique_ptr<cJSON, void (*)(cJSON *item)> MqttConfig::serialize()
{
    // Create a new cJSON object
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

    // Serialize the object
    cJSON_AddStringToObject(root.get(), "brokerAddress", brokerAddress.c_str());
    cJSON_AddStringToObject(root.get(), "deviceId", deviceId.c_str());
    cJSON_AddStringToObject(root.get(), "jwt", jwt.c_str());

    return root;
}

MqttConfig MqttConfig::deserialize(const string &serialized)
{
    // Create a new configuration
    MqttConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.brokerAddress = cJSON_GetObjectItem(root.get(), "brokerAddress")->valuestring;
    config.deviceId = cJSON_GetObjectItem(root.get(), "deviceId")->valuestring;
    config.jwt = cJSON_GetObjectItem(root.get(), "jwt")->valuestring;

    return config;
};
