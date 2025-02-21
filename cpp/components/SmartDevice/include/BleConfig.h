#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include "IDeserializable.h"
#include "ISerializable.h"

namespace SmartDevice
{
    class BleConfig : public IDeserializable, public ISerializable
    {
    public:
        /**
         * @brief Construct a new Ble Config object
         *
         * @param deviceName The device name
         */
        BleConfig(string deviceName) : deviceName(deviceName) {};

        /**
         * @brief Get the device name
         *
         * @return string The device name
         */
        string getDeviceName() { return deviceName; }

        /**
         * @brief Serialize the object to a cJSON object
         *
         * @return unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
         */
        unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "deviceName", deviceName.c_str());
            return unique_ptr<cJSON, void (*)(cJSON *item)>(root, cJSON_Delete);
        }

        /**
         * @brief Deserialize the object from a cJSON object
         *
         * @param root The cJSON object
         * @return unique_ptr<BleConfig> The deserialized object
         */
        static unique_ptr<BleConfig> deserialize(const cJSON *root)
        {
            string deviceName = cJSON_GetStringValue(cJSON_GetObjectItem(root, "deviceName"));
            return make_unique<BleConfig>(deviceName);
        }

    private:
        string deviceName;
    };
};

#endif // BLE_CONFIG_H