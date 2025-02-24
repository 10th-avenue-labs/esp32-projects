#pragma once

#include "IDeserializable.h"
#include "ISerializable.h"
#include "BleConfig.h"
#include "CloudConnectionConfig.h"

#include <string>

namespace SmartDevice
{
    class SmartDeviceConfig : public IDeserializable, public ISerializable
    {
    public:
        std::unique_ptr<BleConfig> bleConfig;                         // The BLE configuration
        std::unique_ptr<CloudConnectionConfig> cloudConnectionConfig; // The cloud connection configuration

        /**
         * @brief Construct a new Smart Device Config object
         *
         * @param bleConfig The BLE configuration
         * @param cloudConnectionConfig The cloud connection configuration
         */
        SmartDeviceConfig(std::unique_ptr<BleConfig> bleConfig, std::unique_ptr<CloudConnectionConfig> cloudConnectionConfig)
            : bleConfig(std::move(bleConfig)), cloudConnectionConfig(std::move(cloudConnectionConfig)) {};

        /**
         * @brief Serialize the object to a cJSON object
         *
         * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
         */
        std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddItemToObject(root, "bleConfig", bleConfig->serialize().release());
            cJSON_AddItemToObject(root, "cloudConnectionConfig", cloudConnectionConfig->serialize().release());
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
            cJSON *bleConfigItem = cJSON_GetObjectItem(root, "bleConfig");
            if (!cJSON_IsObject(bleConfigItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing bleConfig field in JSON");
            }
            auto bleConfigResult = BleConfig::deserialize(bleConfigItem);
            if (!bleConfigResult.isSuccess())
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Failed to deserialize bleConfig: " + bleConfigResult.getError());
            }
            std::unique_ptr<BleConfig> bleConfig(dynamic_cast<BleConfig *>(bleConfigResult.getValue().release()));

            cJSON *cloudConnectionConfigItem = cJSON_GetObjectItem(root, "cloudConnectionConfig");
            if (!cJSON_IsObject(cloudConnectionConfigItem))
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Invalid or missing cloudConnectionConfig field in JSON");
            }
            auto cloudConnectionConfigResult = CloudConnectionConfig::deserialize(cloudConnectionConfigItem);
            if (!cloudConnectionConfigResult.isSuccess())
            {
                return Result<std::unique_ptr<IDeserializable>>::createFailure("Failed to deserialize cloudConnectionConfig: " + cloudConnectionConfigResult.getError());
            }
            std::unique_ptr<CloudConnectionConfig> cloudConnectionConfig(dynamic_cast<CloudConnectionConfig *>(cloudConnectionConfigResult.getValue().release()));

            return Result<std::unique_ptr<IDeserializable>>::createSuccess(std::make_unique<SmartDeviceConfig>(std::move(bleConfig), std::move(cloudConnectionConfig)));
        }
    };
};
