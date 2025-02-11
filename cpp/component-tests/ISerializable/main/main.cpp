#include "ISerializable.h"

#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include "esp_log.h"
#include "esp_event.h"

class MySerializable : public ISerializable
{
public:
    int myInt;                           // Required int
    std::string myString;                // Required string
    bool myBool;                         // Required bool
    unique_ptr<MySerializable> myObject; // Optional object (serializes to null if nullptr)
    std::vector<int> myVector;           // Required array (but cal be empty)

    MySerializable(int myInt, const std::string &myString, bool myBool, unique_ptr<MySerializable> myObject, std::vector<int> myVector)
        : myInt(myInt), myString(myString), myBool(myBool), myObject(std::move(myObject)), myVector(myVector) {}

    unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
    {
        // Provide a simple implementation for the serialize method
        unique_ptr<cJSON, void (*)(cJSON *item)> json(cJSON_CreateObject(), cJSON_Delete);
        // Primitives
        cJSON_AddNumberToObject(json.get(), "myInt", myInt);
        cJSON_AddStringToObject(json.get(), "myString", myString.c_str());
        cJSON_AddBoolToObject(json.get(), "myBool", myBool);

        // Objects

        // This line will add a "myObject": null field if myObject is nullptr
        cJSON_AddItemToObject(json.get(), "myObject", myObject ? myObject->serialize().release() : cJSON_CreateNull());

        // This alternative will not add the "myObject" field if myObject is nullptr and rather omit it completely
        // if (myObject)
        // {
        //     cJSON_AddItemReferenceToObject(json.get(), "myObject", myObject->serialize().release());
        // }

        // Arrays
        cJSON *myVectorArray = cJSON_CreateArray();
        for (int i : myVector)
        {
            cJSON_AddItemToArray(myVectorArray, cJSON_CreateNumber(i));
        }
        cJSON_AddItemToObject(json.get(), "myVector", myVectorArray);

        return json;
    }
};

extern "C" void app_main()
{
    // Create a serializable object without any nesting
    MySerializable mySerializable{42, "Hello, world!", true, nullptr, {1, 2, 3, 4, 5}};

    // Serialize the object to a string
    std::string serialized = mySerializable.serializeToString();
    ESP_LOGI("main", "Serialized: %s", serialized.c_str());

    // Serialize the object to a pretty string
    std::string serializedPretty = mySerializable.serializeToString(true);
    ESP_LOGI("main", "Serialized pretty: %s", serializedPretty.c_str());

    // Create a serializable object with nesting
    MySerializable mySerializableNested{404, "Hello, brave new world!", false, make_unique<MySerializable>(std::move(mySerializable)), {}};

    // Serialize the object to a string
    std::string serializedNested = mySerializableNested.serializeToString();
    ESP_LOGI("main", "Serialized nested: %s", serializedNested.c_str());

    // Serialize the object to a pretty string
    std::string serializedNestedPretty = mySerializableNested.serializeToString(true);
    ESP_LOGI("main", "Serialized nested pretty: %s", serializedNestedPretty.c_str());
}