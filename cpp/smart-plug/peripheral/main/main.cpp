
// #include "Result.h"
// #include "ISerializable.h"

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>

// Define the namespace and key for the plug configuration
#define PLUG_CONFIG_NAMESPACE "storage"
#define PLUG_CONFIG_KEY "config"

static const char *MAIN_TAG = "SMART_PLUG";

// Initialization helpers
// Result<shared_ptr<PlugConfig>> getConfigOrDefault();

// Meta functions
esp_err_t reset();

extern "C" void app_main(void)
{
    // Register the Plug message deserializers

    // Get Smart Plug Config or default

    // Log the configuration
    // ESP_LOGI(TAG, "bleConfig: %s", cJSON_Print(config->serialize().get()));

    // Create a SmartPlug

    // Initiate the smart plug

    // Start the smart plug

    // Initiate the ac dimmer
    // ESP_LOGI(TAG, "initializing ac dimmer");
    // acDimmer = make_unique<AcDimmer>(
    //     config.get()->acDimmerConfig->zcPin,
    //     config.get()->acDimmerConfig->psmPin,
    //     config.get()->acDimmerConfig->debounceUs,
    //     config.get()->acDimmerConfig->offsetLeading,
    //     config.get()->acDimmerConfig->offsetFalling,
    //     config.get()->acDimmerConfig->brightness);
}

////////////////////////////////////////////////////////////////////////////////
/// Initialization helpers
////////////////////////////////////////////////////////////////////////////////

// Result<shared_ptr<PlugConfig>> getConfigOrDefault()
// {
//     // Read the plug configuration from NVS if it exists
//     Result<shared_ptr<PlugConfig>> plugConfigResult = PlugConfig::readPlugConfig(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY);
//     if (!plugConfigResult.isSuccess())
//     {
//         return plugConfigResult;
//     }

//     // Check if a configuration was found
//     if (plugConfigResult.getValue() != nullptr)
//     {
//         ESP_LOGI(TAG, "found plug configuration");
//         return plugConfigResult;
//     }

//     // Create a default plug configuration if one doesn't already exist
//     // TODO: Consider defining the default function somewhere else
//     ESP_LOGI(TAG, "no plug configuration found, creating a default one");
//     config = make_unique<PlugConfig>(
//         PlugConfig{
//             // Config for BLE
//             make_shared<BleConfig>(
//                 "Smart Plug"),
//             // Config for non-dimmable LED
//             make_shared<AcDimmerConfig>(
//                 32,
//                 25,
//                 1000,
//                 5000,
//                 1200,
//                 0),
//             // Config for WiFi
//             nullptr,
//             // Config for MQTT
//             make_shared<MqttConfig>(
//                 "mqtt://10.11.2.96:1883")});

//     return Result<shared_ptr<PlugConfig>>::createSuccess(move(config));
// }

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