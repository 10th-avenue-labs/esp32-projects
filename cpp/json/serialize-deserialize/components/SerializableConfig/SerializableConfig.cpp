#include "SerializableConfig.h"
#include <esp_log.h>

unique_ptr<cJSON, void (*)(cJSON *item)> SerializableConfig::serialize()
{
    // Create the root object
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

    // Add properties to the root object
    cJSON_AddNumberToObject(root.get(), "anInt", anInt);
    cJSON_AddBoolToObject(root.get(), "aBool", aBool);
    cJSON_AddStringToObject(root.get(), "aString", aString.c_str());
    cJSON_AddNumberToObject(root.get(), "aFloat", aFloat);

    // Add array
    cJSON *array = cJSON_CreateArray();
    for (const string &element : aStringVector)
    {
        cJSON_AddItemToArray(array, cJSON_CreateString(element.c_str()));
    }
    cJSON_AddItemToObject(root.get(), "aStringVector", array);

    // Add nested object
    cJSON_AddItemToObject(root.get(), "nested", nestedConfig.serialize().release());

    return root;
}

SerializableConfig SerializableConfig::deserialize(const string &serialized)
{
    // Create a new serializable config object
    SerializableConfig config;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    ESP_LOGI("OBJECT", "here");

    // Deserialize the object
    config.anInt = cJSON_GetObjectItem(root.get(), "anInt")->valueint;
    config.aBool = cJSON_GetObjectItem(root.get(), "aBool")->valueint;
    config.aString = cJSON_GetObjectItem(root.get(), "aString")->valuestring;
    config.aFloat = cJSON_GetObjectItem(root.get(), "aFloat")->valuedouble;

    // Deserialize the array
    cJSON *array = cJSON_GetObjectItem(root.get(), "aStringVector");
    cJSON *element = nullptr;
    cJSON_ArrayForEach(element, array)
    {
        config.aStringVector.push_back(element->valuestring);
    }

    // Deserialize the nested object
    cJSON *nested = cJSON_GetObjectItem(root.get(), "nested");
    config.nestedConfig = NestedSerializableConfig::deserialize(cJSON_Print(nested));

    return config;
}