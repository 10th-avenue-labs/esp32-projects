#pragma once

#include "SmartDeviceConfig.h"
#include "IDeserializable.h"
#include "Result.h"
#include "cJSON.h"

/**
 * @brief Test class for a smart device configuration, extends the base SmartDeviceConfig class with additional fields
 *
 */
class TestDeviceConfig : public SmartDevice::SmartDeviceConfig
{
public:
    int myTestInt;
    float myTestFloat;
    bool myTestBool;
    string myTestString;
    vector<int> myTestVector;

    /**
     * @brief Construct a new TestSmartDeviceConfig object
     *
     * @param myTestInt An integer value
     * @param myTestFloat A float value
     * @param myTestBool A boolean value
     * @param myTestString A string value
     * @param myTestVector A vector of integers
     * @param bleConfig The BLE configuration
     */
    TestDeviceConfig(
        int myTestInt,
        float myTestFloat,
        bool myTestBool,
        string myTestString,
        vector<int> myTestVector,
        std::unique_ptr<SmartDevice::BleConfig> bleConfig)
        : SmartDevice::SmartDeviceConfig(std::move(bleConfig)),
          myTestInt(myTestInt),
          myTestFloat(myTestFloat),
          myTestBool(myTestBool),
          myTestString(myTestString),
          myTestVector(myTestVector) {}

    /**
     * @brief Serialize the object to a cJSON object
     *
     * @return unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
     */
    std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
    {
        // Start with the base class serialization
        std::unique_ptr<cJSON, void (*)(cJSON *item)> root(SmartDevice::SmartDeviceConfig::serialize().release(), cJSON_Delete);

        // Serialize the additional fields for TestSmartDeviceConfig
        cJSON_AddNumberToObject(root.get(), "myTestInt", myTestInt);
        cJSON_AddNumberToObject(root.get(), "myTestFloat", myTestFloat);
        cJSON_AddBoolToObject(root.get(), "myTestBool", myTestBool);

        // Serialize the string, check for an empty value
        cJSON_AddStringToObject(root.get(), "myTestString", myTestString.c_str());

        // Serialize the vector
        cJSON *myTestVectorArray = cJSON_CreateArray();
        for (const int &value : myTestVector)
        {
            cJSON_AddItemToArray(myTestVectorArray, cJSON_CreateNumber(value));
        }
        cJSON_AddItemToObject(root.get(), "myTestVector", myTestVectorArray);

        return root;
    }

    /**
     * @brief Deserialize a TestSmartDeviceConfig object
     *
     * @param root The root cJSON object
     * @return std::unique_ptr<TestSmartDeviceConfig> The deserialized TestSmartDeviceConfig object
     */
    static Result<std::unique_ptr<IDeserializable>> deserialize(const cJSON *root)
    {
        // Deserialize the base class part
        Result<std::unique_ptr<IDeserializable>> baseConfigResult = SmartDevice::SmartDeviceConfig::deserialize(root);
        if (!baseConfigResult.isSuccess())
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Could not serialize SmartDeviceConfig" + baseConfigResult.getError());
        }
        auto deserializedBaseConfig = std::unique_ptr<SmartDevice::SmartDeviceConfig>(dynamic_cast<SmartDevice::SmartDeviceConfig *>(baseConfigResult.getValue().release()));

        // Deserialize the additional fields for TestSmartDeviceConfig
        int myTestInt = cJSON_GetNumberValue(cJSON_GetObjectItem(root, "myTestInt"));
        float myTestFloat = cJSON_GetNumberValue(cJSON_GetObjectItem(root, "myTestFloat"));
        bool myTestBool = cJSON_IsTrue(cJSON_GetObjectItem(root, "myTestBool"));

        // Check for the existence of myTestString and handle potential null
        cJSON *myTestStringItem = cJSON_GetObjectItem(root, "myTestString");
        string myTestString = (myTestStringItem && cJSON_IsString(myTestStringItem)) ? cJSON_GetStringValue(myTestStringItem) : ""; // Default to empty string if not valid

        // Deserialize the vector
        cJSON *myTestVectorRoot = cJSON_GetObjectItem(root, "myTestVector");
        vector<int> myTestVector;
        if (myTestVectorRoot && cJSON_IsArray(myTestVectorRoot))
        {
            cJSON *item = NULL;
            cJSON_ArrayForEach(item, myTestVectorRoot)
            {
                myTestVector.push_back(cJSON_GetNumberValue(item));
            }
        }

        // Extract the BLE config from the base class (already deserialized)
        std::unique_ptr<SmartDevice::BleConfig> bleConfig = std::move(deserializedBaseConfig->bleConfig);

        // Return the fully deserialized object, including the base class and derived fields
        return Result<std::unique_ptr<IDeserializable>>::createSuccess(std::make_unique<TestDeviceConfig>(
            myTestInt,
            myTestFloat,
            myTestBool,
            myTestString,
            myTestVector,
            std::move(bleConfig))); // Pass BLE config to the constructor
    }
};
