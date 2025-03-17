
// #include "Result.h"
// #include "ISerializable.h"
#include "include/SmartPlug.h"
#include "include/SmartPlugConfig.h"
#include "Result.h"
#include "config/BleConfig.h"

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>

// Define the namespace and key for the plug configuration
#define PLUG_CONFIG_NAMESPACE "storage"
#define PLUG_CONFIG_KEY "config"

static const char *MAIN_TAG = "SMART_PLUG";

// Initialization helpers
Result<std::unique_ptr<SmartPlugConfig>> getConfigOrDefault();

// Meta functions
esp_err_t reset();

extern "C" void app_main(void)
{
    // Register deserializers
    IDeserializable::registerDeserializer<AcDimmerConfig>(AcDimmerConfig::deserialize);
    IDeserializable::registerDeserializer<SmartPlugConfig>(SmartPlugConfig::deserialize);

    // Get Smart Plug Config or default
    Result<std::unique_ptr<SmartPlugConfig>> smartPlugConfigResult = getConfigOrDefault();
    if (!smartPlugConfigResult.isSuccess())
    {
        // FIXME: This will likely cause a boot-loop
        ESP_LOGE(MAIN_TAG, "failed to get smart plug config, error: %s", smartPlugConfigResult.getError().c_str());
        return;
    }
    std::unique_ptr<SmartPlugConfig> smartPlugConfig = std::unique_ptr<SmartPlugConfig>(smartPlugConfigResult.getValue().release());

    // Log the configuration
    ESP_LOGI(MAIN_TAG, "Smart Plug Config: %s", smartPlugConfig->serializeToString(true).c_str());

    // // Deserialize the configuration
    // Result<std::unique_ptr<IDeserializable>> deserializedConfigResult = SmartPlugConfig::deserialize(smartPlugConfig->serialize().get());
    // if (!deserializedConfigResult.isSuccess())
    // {
    //     ESP_LOGE(MAIN_TAG, "failed to deserialize smart plug config, error: %s", deserializedConfigResult.getError().c_str());
    //     return;
    // }
    // std::unique_ptr<SmartPlugConfig> deserializedConfig = std::unique_ptr<SmartPlugConfig>(dynamic_cast<SmartPlugConfig *>(deserializedConfigResult.getValue().release()));

    // // Log the deserialized configuration
    // ESP_LOGI(MAIN_TAG, "Deserialized Smart Plug Config Device Name: %s", deserializedConfig->bleConfig->deviceName.c_str());
    // ESP_LOGI(MAIN_TAG, "Deserialized Smart Plug Config Cloud Connection: %s", deserializedConfig->cloudConnectionConfig == nullptr ? "null" : "not null");
    // ESP_LOGI(MAIN_TAG, "Deserialized Smart Plug Config AC Dimmer ZC Pin: %d", deserializedConfig->acDimmerConfig->zcPin);
    // ESP_LOGI(MAIN_TAG, "Deserialized Smart Plug Config AC Dimmer PSM Pin: %d", deserializedConfig->acDimmerConfig->psmPin);
    // ESP_LOGI(MAIN_TAG, "Deserialized Smart Plug Config AC Dimmer Debounce Us: %d", deserializedConfig->acDimmerConfig->debounceUs);
    // ESP_LOGI(MAIN_TAG, "Deserialized Smart Plug Config AC Dimmer Offset Leading: %d", deserializedConfig->acDimmerConfig->offsetLeading);
    // ESP_LOGI(MAIN_TAG, "Deserialized Smart Plug Config AC Dimmer Offset Falling: %d", deserializedConfig->acDimmerConfig->offsetFalling);
    // ESP_LOGI(MAIN_TAG, "Deserialized Smart Plug Config AC Dimmer Brightness: %d", deserializedConfig->acDimmerConfig->brightness);

    // Create a SmartPlug
    SmartPlug smartplug = SmartPlug(std::move(smartPlugConfig));

    // Initiate the smart plug
    smartplug.initialize();

    // Start the smart plug
    // smartplug.start();
}

////////////////////////////////////////////////////////////////////////////////
/// Config update handler
////////////////////////////////////////////////////////////////////////////////

// TODO

////////////////////////////////////////////////////////////////////////////////
/// Initialization helpers
////////////////////////////////////////////////////////////////////////////////

Result<std::unique_ptr<SmartPlugConfig>> getConfigOrDefault()
{
    // Create a default AC Dimmer configuration
    std::unique_ptr<AcDimmerConfig> acDimmerConfig = std::make_unique<AcDimmerConfig>(
        32,   // zcPin
        25,   // psmPin
        1000, // debounceUs
        5000, // offsetLeading
        1200, // offsetFalling
        0     // brightness
    );

    // Create a default plug configuration
    SmartPlugConfig config = SmartPlugConfig(
        std::move(acDimmerConfig),
        make_unique<SmartDevice::BleConfig>("Smart Plug"),
        nullptr);

    return Result<std::unique_ptr<SmartPlugConfig>>::createSuccess(make_unique<SmartPlugConfig>(std::move(config)));

    // // Read the plug configuration from NVS if it exists
    // Result<shared_ptr<PlugConfig>> plugConfigResult = PlugConfig::readPlugConfig(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY);
    // if (!plugConfigResult.isSuccess())
    // {
    //     return plugConfigResult;
    // }

    // // Check if a configuration was found
    // if (plugConfigResult.getValue() != nullptr)
    // {
    //     ESP_LOGI(TAG, "found plug configuration");
    //     return plugConfigResult;
    // }

    // // Create a default plug configuration if one doesn't already exist
    // // TODO: Consider defining the default function somewhere else
    // ESP_LOGI(TAG, "no plug configuration found, creating a default one");
    // config = make_unique<PlugConfig>(
    //     PlugConfig{
    //         // Config for BLE
    //         make_shared<BleConfig>(
    //             "Smart Plug"),
    //         // Config for non-dimmable LED
    //         make_shared<AcDimmerConfig>(
    //             32,
    //             25,
    //             1000,
    //             5000,
    //             1200,
    //             0),
    //         // Config for WiFi
    //         nullptr,
    //         // Config for MQTT
    //         make_shared<MqttConfig>(
    //             "mqtt://10.11.2.96:1883")});

    // return Result<shared_ptr<PlugConfig>>::createSuccess(move(config));
}

////////////////////////////////////////////////////////////////////////////////
/// Meta functions
////////////////////////////////////////////////////////////////////////////////

esp_err_t reset()
{
    // Erase the plug configuration from NVS
    esp_err_t error = nvs_flash_erase();
    if (error != ESP_OK)
    {
        ESP_LOGE(MAIN_TAG, "failed to erase nvs, error code: %s", esp_err_to_name(error));
        return error;
    }

    // Reboot the device
    // FIXME: Enable this in prod on a pin interrupt
    // esp_restart();

    return ESP_OK;
}