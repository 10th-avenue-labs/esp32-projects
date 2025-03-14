#pragma once

#include "config/SmartDeviceConfig.h"
#include "AcDimmerConfig.h"

class SmartPlugConfig : public SmartDevice::SmartDeviceConfig
{
public:
    unique_ptr<AcDimmerConfig> acDimmerConfig;

    SmartPlugConfig(
        std::unique_ptr<AcDimmerConfig> acDimmerConfig,
        std::unique_ptr<SmartDevice::BleConfig> bleConfig,
        std::unique_ptr<SmartDevice::CloudConnectionConfig> cloudConnectionConfig) : SmartDevice::SmartDeviceConfig(std::move(bleConfig),
                                                                                                                    std::move(cloudConnectionConfig)),
                                                                                     acDimmerConfig(std::move(acDimmerConfig)) {};

    /**
     * @brief Serialize the object to a cJSON object
     *
     * @return std::unique_ptr<cJSON, void (*)(cJSON *item)> The serialized cJSON object
     */
    std::unique_ptr<cJSON, void (*)(cJSON *item)> serialize() override
    {
        // Start with the base class serialization
        std::unique_ptr<cJSON, void (*)(cJSON *item)> root(SmartDevice::SmartDeviceConfig::serialize().release(), cJSON_Delete);

        // Serialize the AC Dimmer configuration
        cJSON_AddItemToObject(root.get(), "acDimmerConfig", acDimmerConfig == nullptr ? cJSON_CreateNull() : acDimmerConfig->serialize().release());

        return root;
    }

    /**
     * @brief Deserialize the object from a cJSON object
     *
     * @param root The cJSON object to deserialize from
     * @return Result<std::unique_ptr<IDeserializable>> The deserialized object
     */
    static Result<std::unique_ptr<IDeserializable>> deserialize(const cJSON *root)
    {
        // Deserialize the base class
        Result<std::unique_ptr<IDeserializable>> baseResult = SmartDevice::SmartDeviceConfig::deserialize(root);
        if (!baseResult.isSuccess())
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Could not deserialize base: " + baseResult.getError());
        }
        std::unique_ptr<SmartDeviceConfig> smartDeviceConfig = std::unique_ptr<SmartDeviceConfig>(dynamic_cast<SmartDeviceConfig *>(baseResult.getValue().release()));

        // Deserialize the AC Dimmer configuration
        Result<std::unique_ptr<IDeserializable>> acDimmerConfigResult = IDeserializable::deserializeOptionalObjectItem(root, "acDimmerConfig", AcDimmerConfig::deserialize);
        if (!acDimmerConfigResult.isSuccess())
        {
            return Result<std::unique_ptr<IDeserializable>>::createFailure("Could not deserialize acDimmerConfig: " + acDimmerConfigResult.getError());
        }
        std::unique_ptr<AcDimmerConfig> acDimmerConfig = std::unique_ptr<AcDimmerConfig>(dynamic_cast<AcDimmerConfig *>(acDimmerConfigResult.getValue().release()));

        return Result<std::unique_ptr<IDeserializable>>::createSuccess(std::make_unique<SmartPlugConfig>(
            std::move(acDimmerConfig),
            std::move(smartDeviceConfig->bleConfig),
            std::move(smartDeviceConfig->cloudConnectionConfig)));
    }
};