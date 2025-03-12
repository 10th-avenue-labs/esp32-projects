#pragma once

#include "ISerializable.h"
#include <vector>

using namespace std;

class TestResponse : public ISerializable
{
public:
    int myInt;
    float myFloat;
    bool myBool;
    string myString;
    vector<int> myVector;

    /**
     * @brief Construct a new Test Response object
     *
     * @param myInt The integer value
     * @param myFloat The float value
     * @param myBool The boolean value
     * @param myString The string value
     * @param myVector The vector of integers
     */
    TestResponse(int myInt, float myFloat, bool myBool, string myString, vector<int> myVector)
        : myInt(myInt), myFloat(myFloat), myBool(myBool), myString(myString), myVector(myVector) {}

    /**
     * @brief Serialize the TestResponse object
     *
     * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
     */
    std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
    {
        // Create the root cJSON object
        std::unique_ptr<cJSON, void (*)(cJSON *item)> root(cJSON_CreateObject(), cJSON_Delete);

        // Add the fields
        cJSON_AddNumberToObject(root.get(), "myInt", myInt);
        cJSON_AddNumberToObject(root.get(), "myFloat", myFloat);
        cJSON_AddBoolToObject(root.get(), "myBool", myBool);
        cJSON_AddStringToObject(root.get(), "myString", myString.c_str());

        // Create the array for the vector
        cJSON *myVectorArray = cJSON_CreateArray();
        for (const int &value : myVector)
        {
            cJSON_AddItemToArray(myVectorArray, cJSON_CreateNumber(value));
        }
        cJSON_AddItemToObject(root.get(), "myVector", myVectorArray);

        return root;
    }
};