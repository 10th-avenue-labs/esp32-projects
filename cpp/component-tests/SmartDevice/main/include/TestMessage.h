#pragma once

#include "IDeserializable.h"

#include <string>
#include <vector>
#include <esp_log.h>

static const char *TEST_MESSAGE_TAG = "TEST_MESSAGE";

using namespace std;

/**
 * @brief A test message class
 *
 */
class TestMessage : public IDeserializable
{
public:
    int myInt;
    float myFloat;
    bool myBool;
    string myString;
    vector<int> myVector;

    TestMessage(int myInt, float myFloat, bool myBool, string myString, vector<int> myVector)
        : myInt(myInt), myFloat(myFloat), myBool(myBool), myString(myString), myVector(myVector) {}

    /**
     * @brief Deserialize a TestMessage object
     *
     * @param root The root cJSON object
     * @return std::unique_ptr<IDeserializable> The deserialized TestMessage object
     */
    static Result<std::unique_ptr<IDeserializable>> deserialize(const cJSON *root)
    {
        ESP_LOGI(TEST_MESSAGE_TAG, "deserializing TestMessage");

        if (!root)
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Root JSON object is null");
        }

        cJSON *intItem = cJSON_GetObjectItem(root, "myInt");
        cJSON *floatItem = cJSON_GetObjectItem(root, "myFloat");
        cJSON *boolItem = cJSON_GetObjectItem(root, "myBool");
        cJSON *stringItem = cJSON_GetObjectItem(root, "myString");
        cJSON *vectorItem = cJSON_GetObjectItem(root, "myVector");

        if (!cJSON_IsNumber(intItem) || !cJSON_IsNumber(floatItem) || !cJSON_IsBool(boolItem))
        {
            // TODO: Parsing failing here for some reason
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing fields in JSON");
        }

        int myInt = intItem->valueint;
        float myFloat = static_cast<float>(floatItem->valuedouble);
        bool myBool = cJSON_IsTrue(boolItem);

        std::string myString;
        if (cJSON_IsString(stringItem) && stringItem->valuestring)
        {
            myString = stringItem->valuestring;
        }
        else
        {
            ESP_LOGW(TEST_MESSAGE_TAG, "myString is missing or not a valid string, defaulting to empty");
            myString = "";
        }

        std::vector<int> myVector;
        if (cJSON_IsArray(vectorItem))
        {
            cJSON *item = nullptr;
            cJSON_ArrayForEach(item, vectorItem)
            {
                if (cJSON_IsNumber(item))
                {
                    myVector.push_back(item->valueint);
                }
                else
                {
                    ESP_LOGW(TEST_MESSAGE_TAG, "Skipping invalid element in myVector");
                }
            }
        }
        else
        {
            ESP_LOGW(TEST_MESSAGE_TAG, "myVector is missing or not an array");
        }

        return Result<std::unique_ptr<IDeserializable>>::createSuccess(std::make_unique<TestMessage>(myInt, myFloat, myBool, myString, myVector));
    }
};