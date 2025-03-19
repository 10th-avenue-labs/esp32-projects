#include "include/SmartPlug.h"
#include "include/SmartPlugConfig.h"
#include "Result.h"
#include "config/BleConfig.h"

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <nvs_handle.hpp>

// Define the namespace and key for the plug configuration
#define PLUG_CONFIG_NAMESPACE "storage"
#define PLUG_CONFIG_KEY "config"

// Default plug configuration
SmartPlugConfig defaultConfig = SmartPlugConfig(
    std::make_unique<AcDimmerConfig>(
        32,   // zcPin
        25,   // psmPin
        1000, // debounceUs
        5000, // offsetLeading
        1200, // offsetFalling
        0     // brightness
        ),
    std::make_unique<SmartDevice::BleConfig>(
        "Smart Plug" // Device Name
        ),
    nullptr // Cloud Connection
);

// NVS helpers
Result<> writeNvsBlob(const string &namespaceValue, const string &key, const string &blob);
Result<std::unique_ptr<std::string>> readNvsBlob(const string &nvsNamespace, const string &key);
Result<> resetNvs();

// Config update handler
void configUpdatedHandler(std::unique_ptr<SmartPlugConfig> config);

// Initialization helpers
Result<std::unique_ptr<SmartPlugConfig>> getConfigOrDefault();

static const char *MAIN_TAG = "MAIN";

extern "C" void app_main(void)
{
    // Register deserializers for Smart Device
    IDeserializable::registerDeserializer<SmartDevice::SmartDeviceConfig>(SmartDevice::SmartDeviceConfig::deserialize);
    IDeserializable::registerDeserializer<SmartDevice::BleConfig>(SmartDevice::BleConfig::deserialize);
    IDeserializable::registerDeserializer<SmartDevice::CloudConnectionConfig>(SmartDevice::CloudConnectionConfig::deserialize);
    IDeserializable::registerDeserializer<SmartDevice::Request>(SmartDevice::Request::deserialize);

    // Register deserializers for Smart Plug
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

    // Create a SmartPlug
    SmartPlug smartplug = SmartPlug(std::move(smartPlugConfig), configUpdatedHandler);

    // Initiate the smart plug
    smartplug.initialize();

    // Start the smart plug
    smartplug.start();

    // Wait indefinitely
    // TODO: Implement a way to stop the task
    while (true)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// NVS helpers
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Write a blob to the NVS
 *
 * @param namespaceValue The namespace to write to
 * @param key The key to write the blob to
 * @param blob The blob to write
 * @return Result<> A result indicating success or failure
 */
Result<> writeNvsBlob(const string &namespaceValue, const string &key, const string &blob)
{
    // Get a handle to the Namespace in the NVS
    esp_err_t error;
    unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(
        namespaceValue.c_str(), // Namespace
        NVS_READWRITE,          // Open mode
        &error                  // Error code
    );
    if (error != ESP_OK)
    {
        return Result<>::createFailure(format("failed to open nvs handle: {}", esp_err_to_name(error)));
    }

    // Write the serialized plug configuration to the NVS
    error = handle->set_blob(key.c_str(), blob.c_str(), blob.length());
    if (error != ESP_OK)
    {
        return Result<>::createFailure(format("failed to write blob with key '{}' to nvs: {}", key.c_str(), esp_err_to_name(error)));
    }

    // Commit the changes
    error = handle->commit();
    if (error != ESP_OK)
    {
        return Result<>::createFailure(format("failed to commit changes to nvs: {}", esp_err_to_name(error)));
    }

    return Result<>::createSuccess();
}

/**
 * @brief Read a blob from the NVS
 *
 * @param nvsNamespace The namespace to read from
 * @param key The key to read the blob from
 * @return Result<std::unique_ptr<std::string>> A result containing the blob or an error message. Nullptr if the key is not initialized
 */
Result<std::unique_ptr<std::string>> readNvsBlob(const string &nvsNamespace, const string &key)
{
    // Initialise the non-volatile flash storage (NVS)
    ESP_LOGI(MAIN_TAG, "initializing nvs flash");
    esp_err_t response = nvs_flash_init();

    // Attempt to recover if an error occurred
    if (response != ESP_OK)
    {
        // Check if a recoverable error occured
        if (response == ESP_ERR_NVS_NO_FREE_PAGES ||
            response == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            // Erase and re-try if necessary
            // Note: This will erase the nvs flash.
            // TODO: We should consider alternative impls here. Erasing the NVS could be a very unwanted side-effect
            ESP_LOGI(MAIN_TAG, "erasing nvs flash");
            ESP_ERROR_CHECK(nvs_flash_erase());
            response = nvs_flash_init();

            if (response != ESP_OK)
            {
                return Result<std::unique_ptr<std::string>>::createFailure(format("failed to initialize nvs flash: {}", esp_err_to_name(response)));
            };
        }
    }

    // Get a handle to the Namespace in the NVS
    esp_err_t error;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(
        nvsNamespace.c_str(), // Namespace
        NVS_READWRITE,        // Open mode
        &error                // Error code
    );
    if (error != ESP_OK)
    {
        return Result<std::unique_ptr<std::string>>::createFailure(format("failed to open nvs handle: {}", esp_err_to_name(error)));
    }

    // Read the size of the blob from the NVS
    size_t size = 0;
    error = handle->get_item_size(nvs::ItemType::BLOB, key.c_str(), size);
    switch (error)
    {
    case ESP_OK:
        break;
    case ESP_ERR_NVS_NOT_FOUND:
    {
        ESP_LOGI(MAIN_TAG, "the key '%s' is not initialized yet", key.c_str());

        return Result<std::unique_ptr<std::string>>::createSuccess(nullptr);
        break;
    }
    default:
        return Result<std::unique_ptr<std::string>>::createFailure(format("failed to read blob size: {}", esp_err_to_name(error)));
    }

    // Read the blob from the NVS
    void *blob = malloc(size + 1); // We add 1 to fit the null terminator at the end
    error = handle->get_blob(key.c_str(), blob, size);
    if (error != ESP_OK)
    {
        return Result<std::unique_ptr<std::string>>::createFailure(format("failed to read blob with key '{}' from nvs: {}", key.c_str(), esp_err_to_name(error)));
    }
    if (blob == nullptr)
    {
        return Result<std::unique_ptr<std::string>>::createFailure("failed to read blob from nvs");
    }

    // Add the null terminator to the blob
    ((char *)blob)[size] = '\0';

    // Cast the blob to a string
    std::string blobString = std::string((char *)blob);

    // Deallocate the blob
    free(blob);

    return Result<std::unique_ptr<std::string>>::createSuccess(std::make_unique<std::string>(blobString));
}

/**
 * @brief Reset the NVS
 *
 * @return Result<> A result indicating success or failure
 */
Result<> resetNvs()
{
    // Erase the plug configuration from NVS
    esp_err_t error = nvs_flash_erase();
    if (error != ESP_OK)
    {
        return Result<>::createFailure(format("failed to erase nvs: {}", esp_err_to_name(error)));
    }

    // Reboot the device
    // FIXME: Enable this in prod on a pin interrupt
    // esp_restart();

    return Result<>::createSuccess();
}

////////////////////////////////////////////////////////////////////////////////
/// Config update handler
////////////////////////////////////////////////////////////////////////////////

void configUpdatedHandler(unique_ptr<SmartPlugConfig> config)
{
    ESP_LOGI(MAIN_TAG, "config updated");

    // Serialize the configuration
    string serializedConfig = config->serializeToString();

    // Write the serialized configuration to the NVS
    Result<> writeResult = writeNvsBlob(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY, serializedConfig);
    if (!writeResult.isSuccess())
    {
        ESP_LOGE(MAIN_TAG, "failed to write config to nvs: %s", writeResult.getError().c_str());
        return;
    }

    ESP_LOGI(MAIN_TAG, "config written to nvs: %s", serializedConfig.c_str());
}

////////////////////////////////////////////////////////////////////////////////
/// Initialization helpers
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Get the Smart Plug configuration from the NVS or the  default if it doesn't exist
 *
 * @return Result<std::unique_ptr<SmartPlugConfig>> A result containing the configuration or an error message
 */
Result<std::unique_ptr<SmartPlugConfig>> getConfigOrDefault()
{
    // Read the existing config from NVS if it exists
    Result<std::unique_ptr<string>> existingConfigResult = readNvsBlob(PLUG_CONFIG_NAMESPACE, PLUG_CONFIG_KEY);
    if (!existingConfigResult.isSuccess())
    {
        return Result<std::unique_ptr<SmartPlugConfig>>::createFailure(format("failed to read existing config: {}", existingConfigResult.getError()));
    }

    // Take ownership of the result value
    std::unique_ptr<string> existingConfig = existingConfigResult.getValue();

    // Check if no configuration was found
    if (existingConfig == nullptr)
    {
        ESP_LOGI(MAIN_TAG, "no existing configuration found, using default configuration");

        return Result<std::unique_ptr<SmartPlugConfig>>::createSuccess(std::make_unique<SmartPlugConfig>(std::move(defaultConfig)));
    }

    // Parse the serialized configuration as json
    const cJSON *json = cJSON_Parse(existingConfig->c_str());
    if (json == nullptr)
    {
        return Result<std::unique_ptr<SmartPlugConfig>>::createFailure("failed to parse json");
    }

    // Deserialize the plug configuration
    Result<std::unique_ptr<IDeserializable>> configResult = SmartPlugConfig::deserialize(json);
    if (!configResult.isSuccess())
    {
        return Result<std::unique_ptr<SmartPlugConfig>>::createFailure(format("failed to deserialize plug config: {}", configResult.getError()));
    }

    ESP_LOGI(MAIN_TAG, "successfully loaded existing configuration");
    return Result<std::unique_ptr<SmartPlugConfig>>::createSuccess(std::unique_ptr<SmartPlugConfig>(dynamic_cast<SmartPlugConfig *>(configResult.getValue().release())));
}