#include "NestedSerializableConfig.h"

unique_ptr<cJSON, void (*)(cJSON *item)> NestedSerializableConfig::serialize()
{
    // Create the root object
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

    // Add properties to the root object
    cJSON_AddNumberToObject(root.get(), "anInt", anInt);
    cJSON_AddBoolToObject(root.get(), "aBool", aBool);
    cJSON_AddStringToObject(root.get(), "aString", aString.c_str());

    // Format float to desired precision
    char floatBuffer[16];
    snprintf(floatBuffer, sizeof(floatBuffer), "%.6f", aFloat);
    cJSON_AddRawToObject(root.get(), "aFloat", floatBuffer);

    return root;
}

NestedSerializableConfig NestedSerializableConfig::deserialize(const string &serialized)
{
    // Create a new serializable config object
    NestedSerializableConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    config.anInt = cJSON_GetObjectItem(root.get(), "anInt")->valueint;
    config.aBool = cJSON_GetObjectItem(root.get(), "aBool")->valueint;
    config.aString = cJSON_GetObjectItem(root.get(), "aString")->valuestring;
    config.aFloat = cJSON_GetObjectItem(root.get(), "aFloat")->valuedouble;

    return config;
}