#ifndef SMART_DEVICE_CONFIG_H
#define SMART_DEVICE_CONFIG_H

#include "BleConfig.h"
#include "IDeserializable.h"

#include <map>
#include <string>
#include <functional>

namespace SmartDevice
{
    class SmartDeviceConfig : public IDeserializable, public ISerializable
    {
    public:
        std::unique_ptr<BleConfig> bleConfig; // The BLE configuration
        // TODO: Wifi
        // TODO: MQTT

        SmartDeviceConfig(std::unique_ptr<BleConfig> bleConfig) : bleConfig(std::move(bleConfig)) {}

        /**
         * @brief Serialize a SmartDeviceConfig object
         *
         * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
         */
        std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddItemToObject(root, "bleConfig", bleConfig->serialize().release());
            return std::unique_ptr<cJSON, void (*)(cJSON *item)>(root, cJSON_Delete);
        }

        /**
         * @brief Deserialize a SmartDeviceConfig object
         *
         * @param root The root cJSON object
         * @return std::unique_ptr<SmartDeviceConfig> The deserialized SmartDeviceConfig object
         */
        static Result<std::unique_ptr<IDeserializable>> deserialize(const cJSON *root)
        {
            cJSON *bleConfigRoot = cJSON_GetObjectItem(root, "bleConfig");
            std::unique_ptr<BleConfig> bleConfig = BleConfig::deserialize(bleConfigRoot);
            return Result<std::unique_ptr<IDeserializable>>::createSuccess(std::make_unique<SmartDeviceConfig>(std::move(bleConfig)));
        }
    };

    // HACK: Ensures the deserializer is registered when the SmartDeviceConfig class is included
    static bool smartDeviceConfigRegistered = (IDeserializable::registerDeserializer<SmartDeviceConfig>(SmartDeviceConfig::deserialize), true);
};

#endif // SMART_DEVICE_CONFIG_H