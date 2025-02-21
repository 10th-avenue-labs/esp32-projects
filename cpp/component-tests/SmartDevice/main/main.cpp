#include <iostream>
#include <string>

#include "SmartDevice.h"
#include "esp_log.h"
#include "esp_event.h"

using namespace std;

static const char *TAG = "MAIN";

/**
 * @brief Test class for a smart device configuration, extends the base SmartDeviceConfig class with additional fields
 *
 */
class TestSmartDeviceConfig : public SmartDevice::SmartDeviceConfig
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
    TestSmartDeviceConfig(
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
    static Result<std::unique_ptr<TestSmartDeviceConfig>> deserialize(const cJSON *root)
    {
        // Deserialize the base class part
        Result<std::unique_ptr<IDeserializable>> baseConfigResult = SmartDevice::SmartDeviceConfig::deserialize(root);
        if (!baseConfigResult.isSuccess())
        {
            return Result<std::unique_ptr<TestSmartDeviceConfig>>::createFailure("Could not serialize SmartDeviceConfig" + baseConfigResult.getError());
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
        return Result<std::unique_ptr<TestSmartDeviceConfig>>::createSuccess(std::make_unique<TestSmartDeviceConfig>(
            myTestInt,
            myTestFloat,
            myTestBool,
            myTestString,
            myTestVector,
            std::move(bleConfig))); // Pass BLE config to the constructor
    }
};

// // HACK: Ensures the deserializer is registered
// static bool testSmartDeviceConfigRegistered = (IDeserializable::registerDeserializer<TestSmartDeviceConfig>(TestSmartDeviceConfig::deserialize), true);

// /**
//  * @brief A test message class
//  *
//  */
// class TestMessage : public IDeserializable
// {
// public:
//     int myInt;
//     float myFloat;
//     bool myBool;
//     string myString;
//     vector<int> myVector;

//     TestMessage(int myInt, float myFloat, bool myBool, string myString, vector<int> myVector)
//         : myInt(myInt), myFloat(myFloat), myBool(myBool), myString(myString), myVector(myVector) {}

//     /**
//      * @brief Deserialize a TestMessage object
//      *
//      * @param root The root cJSON object
//      * @return std::unique_ptr<IDeserializable> The deserialized TestMessage object
//      */
//     static Result<std::unique_ptr<TestMessage>> deserialize(const cJSON *root)
//     {
//         ESP_LOGI(TAG, "deserializing TestMessage");

//         if (!root)
//         {
//             return Result<std::unique_ptr<TestMessage>>::createFailure("Root JSON object is null");
//         }

//         cJSON *intItem = cJSON_GetObjectItem(root, "myInt");
//         cJSON *floatItem = cJSON_GetObjectItem(root, "myFloat");
//         cJSON *boolItem = cJSON_GetObjectItem(root, "myBool");
//         cJSON *stringItem = cJSON_GetObjectItem(root, "myString");
//         cJSON *vectorItem = cJSON_GetObjectItem(root, "myVector");

//         if (!cJSON_IsNumber(intItem) || !cJSON_IsNumber(floatItem) || !cJSON_IsBool(boolItem))
//         {
//             // TODO: Parsing failing here for some reason
//             return Result<std::unique_ptr<TestMessage>>::createFailure("Invalid or missing fields in JSON");
//         }

//         int myInt = intItem->valueint;
//         float myFloat = static_cast<float>(floatItem->valuedouble);
//         bool myBool = cJSON_IsTrue(boolItem);

//         std::string myString;
//         if (cJSON_IsString(stringItem) && stringItem->valuestring)
//         {
//             myString = stringItem->valuestring;
//         }
//         else
//         {
//             ESP_LOGW(TAG, "myString is missing or not a valid string, defaulting to empty");
//             myString = "";
//         }

//         std::vector<int> myVector;
//         if (cJSON_IsArray(vectorItem))
//         {
//             cJSON *item = nullptr;
//             cJSON_ArrayForEach(item, vectorItem)
//             {
//                 if (cJSON_IsNumber(item))
//                 {
//                     myVector.push_back(item->valueint);
//                 }
//                 else
//                 {
//                     ESP_LOGW(TAG, "Skipping invalid element in myVector");
//                 }
//             }
//         }
//         else
//         {
//             ESP_LOGW(TAG, "myVector is missing or not an array");
//         }

//         return Result<std::unique_ptr<TestMessage>>::createSuccess(std::make_unique<TestMessage>(myInt, myFloat, myBool, myString, myVector));
//     }
// };

// // HACK: Ensures the deserializer is registered
// static bool testMessageRegistered = (IDeserializable::registerDeserializer<TestMessage>(TestMessage::deserialize), true);

// /**
//  * @brief Test class for a smart device
//  *
//  */
// class TestSmartDevice : public SmartDevice::SmartDevice
// {
// public:
//     TestSmartDevice(TestSmartDeviceConfig config) : SmartDevice::SmartDevice(std::move(config))
//     {
//         // Register event handlers
//         registerMessageHandler("testMessage", std::bind(&TestSmartDevice::handleTestMessage, this, std::placeholders::_1));
//         registerMessageHandler("testMessage2", std::bind(&TestSmartDevice::handleTestMessage2, this, std::placeholders::_1));
//     };

//     Result<shared_ptr<ISerializable>> handleTestMessage(unique_ptr<IDeserializable> message)
//     {
//         ESP_LOGI(TAG, "Handling test message 1");
//         return Result<shared_ptr<ISerializable>>::createFailure("Not implemented");
//     }

//     Result<shared_ptr<ISerializable>> handleTestMessage2(unique_ptr<IDeserializable> message)
//     {
//         ESP_LOGI(TAG, "Handling test message 2");
//         return Result<shared_ptr<ISerializable>>::createFailure("Not implemented");
//     }
// };

extern "C" void app_main()
{
    ///////////////////////////////////////////////////////////////////////////
    // Test code for serialization and deserialization
    ///////////////////////////////////////////////////////////////////////////

    // Create a SmartDeviceConfig object
    SmartDevice::SmartDeviceConfig config(make_unique<SmartDevice::BleConfig>("My Device"));

    // Serialize the config
    auto serializedConfig = config.serializeToString(true);
    ESP_LOGI(TAG, "serialized config: %s", serializedConfig.c_str());

    // Deserialize the config
    Result<std::unique_ptr<IDeserializable>> deserializedConfigResult = SmartDevice::SmartDeviceConfig::deserialize(config.serialize().get());
    if (!deserializedConfigResult.isSuccess())
    {
        ESP_LOGE(TAG, "Failed to deserialize config: %s", deserializedConfigResult.getError().c_str());
        return;
    }
    auto deserializedConfig = std::unique_ptr<SmartDevice::SmartDeviceConfig>(dynamic_cast<SmartDevice::SmartDeviceConfig *>(deserializedConfigResult.getValue().release()));
    ESP_LOGI(TAG, "deserialized config: %s", deserializedConfig->bleConfig->getDeviceName().c_str());

    // Create a TestSmartDeviceConfig object
    TestSmartDeviceConfig testConfig(42, 3.14, true, "Hello, world!", {1, 2, 3, 4, 5}, make_unique<SmartDevice::BleConfig>("My New Device"));

    // Serialize the test config
    auto serializedTestConfig = testConfig.serializeToString(true);
    ESP_LOGI(TAG, "serialized test config: %s", serializedTestConfig.c_str());

    // Deserialize the test config
    auto deserializedTestConfigResult = TestSmartDeviceConfig::deserialize(testConfig.serialize().get());
    if (!deserializedTestConfigResult.isSuccess())
    {
        ESP_LOGE(TAG, "Failed to deserialize test config: %s", deserializedTestConfigResult.getError().c_str());
        return;
    }
    auto deserializedTestConfig = deserializedTestConfigResult.getValue();
    ESP_LOGI(TAG, "deserialized test config deviceName: %s", deserializedTestConfig->bleConfig->getDeviceName().c_str());
    ESP_LOGI(TAG, "deserialized test config myTestInt: %d", deserializedTestConfig->myTestInt);
    ESP_LOGI(TAG, "deserialized test config myTestFloat: %f", deserializedTestConfig->myTestFloat);
    ESP_LOGI(TAG, "deserialized test config myTestBool: %s", deserializedTestConfig->myTestBool ? "true" : "false");
    ESP_LOGI(TAG, "deserialized test config myTestString: %s", deserializedTestConfig->myTestString.c_str());
    ESP_LOGI(TAG, "deserialized test config myTestVector:");
    for (int i = 0; i < deserializedTestConfig->myTestVector.size(); i++)
    {
        ESP_LOGI(TAG, "  %d", deserializedTestConfig->myTestVector[i]);
    }

    // ///////////////////////////////////////////////////////////////////////////
    // // Test code for the smart device event handling
    // ///////////////////////////////////////////////////////////////////////////

    // ESP_LOGI(TAG, "test code for the smart device event handling\n\n");

    // // Create a TestSmartDevice object
    // TestSmartDevice device(std::move(*deserializedTestConfig));

    // // Create a TestMessage string payload
    // string testMessageString = R"({
    //     "type": "testMessage1",
    //     "data": {
    //         "myInt": 42,
    //         "myFloat": 3.14,
    //         "myBool": true,
    //         "myString": "Hello, world!",
    //         "myVector": [1, 2, 3, 4, 5]
    //     }
    // })";
    // ESP_LOGI(TAG, "Test message string: %s", testMessageString.c_str());

    // // Deserialize the TestMessage object
    // auto testMessageDeserializer = IDeserializable::getDeserializer<TestMessage>();
    // auto deserializedResult = testMessageDeserializer(cJSON_Parse(testMessageString.c_str()));
    // if (!deserializedResult.isSuccess())
    // {
    //     ESP_LOGE(TAG, "Failed to deserialize test message: %s", deserializedResult.getError().c_str());
    //     return;
    // }
    // auto deserialized = deserializedResult.getValue();
    // ESP_LOGI(TAG, "deserialized test message");
    // TestMessage testMessage = dynamic_cast<TestMessage &>(*deserialized);
    // ESP_LOGI(TAG, "deserialized test message myInt: %d", testMessage.myInt);
    // ESP_LOGI(TAG, "deserialized test message myFloat: %f", testMessage.myFloat);
    // ESP_LOGI(TAG, "deserialized test message myBool: %s", testMessage.myBool ? "true" : "false");
    // ESP_LOGI(TAG, "deserialized test message myString: %s", testMessage.myString.c_str());
    // ESP_LOGI(TAG, "deserialized test message myVector:");
    // for (int i = 0; i < testMessage.myVector.size(); i++)
    // {
    //     ESP_LOGI(TAG, "  %d", testMessage.myVector[i]);
    // }

    // // Create a TestSmartDeviceConfig object
    // TestSmartDeviceConfig config;

    /**
     * Things needed for a type of smart device:
     * 1. Config applicable to just the smart device type. i.e. an ac dimmer would have a config to store properties like the brightness value
     * 2. Event handlers for the smart device type. i.e. an ac dimmer would have an event handler for the brightness value changing
     * 3. Serializers and deserializers for the config and message types. There's a lot of overlap here, but not necessarily 100% overlap
     * 4. Potential overrides of functions? Maybe like the init to initialize an ac dimmer?
     */

    ///////////////////////////////////////////////////////////////////////////
    // Event handlers for the smart device type
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    // Serializers and deserializers for the config and message types
    ///////////////////////////////////////////////////////////////////////////

    // Should we have seperate init and start functions? What would they each do? Should the caller or the device be responsible for the initial config?

    // // Register deserializers for the smart device (Should this happen automatically?)
    // IDeserializable::registerDeserializer<SmartDevice::BleConfig>(SmartDevice::BleConfig::deserialize);

    // // Register deserializers for the extended smart device
    // IDeserializable::registerDeserializer<TestSmartDeviceConfig>(TestSmartDeviceConfig::deserialize);

    // Read the config

    // Need to register deserializers to read config

    // TestSmartDevice device();

    // device.start();
}