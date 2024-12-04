#include "AcDimmerConfig.h"

unique_ptr<cJSON, void (*)(cJSON *item)> AcDimmerConfig::serialize() {
    // Create a new cJSON object
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

    // Serialize the object
    cJSON_AddNumberToObject(root.get(), "zcPin", zcPin);
    cJSON_AddNumberToObject(root.get(), "psmPin", psmPin);
    cJSON_AddNumberToObject(root.get(), "debounceUs", debounceUs);
    cJSON_AddNumberToObject(root.get(), "offsetLeading", offsetLeading);
    cJSON_AddNumberToObject(root.get(), "offsetFalling", offsetFalling);
    cJSON_AddNumberToObject(root.get(), "brightness", brightness);

    return root;
}

AcDimmerConfig AcDimmerConfig::deserialize(const string& serialized) {
    // Create a new configuration
    AcDimmerConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.zcPin = cJSON_GetObjectItem(root.get(), "zcPin")->valueint;
    config.psmPin = cJSON_GetObjectItem(root.get(), "psmPin")->valueint;
    config.debounceUs = cJSON_GetObjectItem(root.get(), "debounceUs")->valueint;
    config.offsetLeading = cJSON_GetObjectItem(root.get(), "offsetLeading")->valueint;
    config.offsetFalling = cJSON_GetObjectItem(root.get(), "offsetFalling")->valueint;
    config.brightness = cJSON_GetObjectItem(root.get(), "brightness")->valueint;

    return config;
};
