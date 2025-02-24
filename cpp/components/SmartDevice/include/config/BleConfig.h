#pragma once

#include "IDeserializable.h"
#include "ISerializable.h"

#include <string>

namespace SmartDevice
{
    class BleConfig : public IDeserializable, public ISerializable
    {
    public:
        std::string deviceName;

        /**
         * @brief Construct a new Ble Config object
         *
         * @param deviceName The device name
         */
        BleConfig(std::string deviceName) : deviceName(deviceName) {};

        /**
         * @brief Copy constructor
         *
         * @param other The other BleConfig object
         */
        BleConfig(const BleConfig &other) : deviceName(other.deviceName) {};

        /**
         * @brief Serialize the object to a cJSON object
         *
         * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
         */
        std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "deviceName", deviceName.c_str());
            return std::unique_ptr<cJSON, void (*)(cJSON *item)>(root, cJSON_Delete);
        }

        /**
         * @brief Deserialize the object from a cJSON object
         *
         * @param root The cJSON object
         * @return Result<std::unique_ptr<IDeserializable>> The deserialized object
         */
        static Result<std::unique_ptr<IDeserializable>> deserialize(const cJSON *root)
        {
            cJSON *deviceNameItem = cJSON_GetObjectItem(root, "deviceName");
            if (!cJSON_IsString(deviceNameItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing deviceName field in JSON");
            }
            return Result<std::unique_ptr<IDeserializable>>::createSuccess(std::make_unique<BleConfig>(deviceNameItem->valuestring));
        }
    };
};
