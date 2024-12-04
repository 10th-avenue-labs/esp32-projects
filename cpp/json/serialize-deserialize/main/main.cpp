#include "SerializableConfig.h"
#include <iostream>
#include <esp_log.h>

const char* TAG = "main";

extern "C" void app_main(){
    // Create a serializable config object
    ESP_LOGI(TAG, "creating a serializable config object");
    SerializableConfig config = {
        .anInt = 42,
        .aFloat = 3.14f,
        .aBool = true,
        .aString = "Hello, World!",
        .aStringVector = {"one", "two", "three"},
        .nestedConfig = {
            .anInt = -40,
            .aFloat = 2.71828,
            .aBool = false,
            .aString = "Hello, Mars!"
        }
    };

    // Serialize the object
    ESP_LOGI(TAG, "serializing the object");
    string serialized = cJSON_Print(config.serialize().get());

    // Print the serialized object
    ESP_LOGI(TAG, "serialized object: %s", serialized.c_str());

    // Deserialize the object
    ESP_LOGI(TAG, "deserializing the object");
    SerializableConfig deserialized = SerializableConfig::deserialize(serialized);

    // Print the deserialized object
    ESP_LOGI(TAG, "deserialized object: %d, %f, %d, %s", deserialized.anInt, deserialized.aFloat, deserialized.aBool, deserialized.aString.c_str());

    // Print the deserialized array
    ESP_LOGI(TAG, "deserialized array:");
    for (const string &element : deserialized.aStringVector)
    {
        ESP_LOGI(TAG, "%s", element.c_str());
    }

    // Print the deserialized nested object
    ESP_LOGI(TAG, "deserialized nested object: %d, %f, %d, %s", deserialized.nestedConfig.anInt, deserialized.nestedConfig.aFloat, deserialized.nestedConfig.aBool, deserialized.nestedConfig.aString.c_str());
}