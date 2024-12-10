#include "PlugConfig.h"

static const char* TAG = "PLUG_CONFIG";

Result<shared_ptr<PlugConfig>> PlugConfig::readPlugConfig(const string& namespaceValue, const string& key) {
    // Initialise the non-volatile flash storage (NVS)
    ESP_LOGI(TAG, "initializing nvs flash");
    esp_err_t response = nvs_flash_init();

    // Attempt to recover if an error occurred
    if (response != ESP_OK) {
        // Check if a recoverable error occured
        if (response == ESP_ERR_NVS_NO_FREE_PAGES ||
            response == ESP_ERR_NVS_NEW_VERSION_FOUND) {

            // Erase and re-try if necessary
            // Note: This will erase the nvs flash.
            // TODO: We should consider alternative impls here. Erasing the NVS could be a very unwanted side-effect
            ESP_LOGI(TAG, "erasing nvs flash");
            ESP_ERROR_CHECK(nvs_flash_erase());
            response = nvs_flash_init();

            if (response != ESP_OK) {
                return Result<shared_ptr<PlugConfig>>::createFailure(format("failed to initialize nvs flash: {}", esp_err_to_name(response)));
            };
        }
    }

    // Get a handle to the Namespace in the NVS
    esp_err_t error;
    unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(
        namespaceValue.c_str(), // Namespace
        NVS_READWRITE,          // Open mode
        &error                  // Error code
    );
    if (error != ESP_OK) {
        return Result<shared_ptr<PlugConfig>>::createFailure(format("failed to open nvs handle: {}", esp_err_to_name(error)));
    }

    // Read the size of the serialized plug configuration from the NVS
    size_t size = 0;
    error = handle->get_item_size(nvs::ItemType::BLOB, key.c_str(), size);
    switch (error) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND: {
            ESP_LOGI(TAG, "the key '%s' is not initialized yet", key.c_str());

            return Result<shared_ptr<PlugConfig>>::createSuccess(nullptr);
            break;
        }
        default:
            return Result<shared_ptr<PlugConfig>>::createFailure(format("failed to read blob size: {}", esp_err_to_name(error)));
    }

    // Read the serialized plug configuration from the NVS
    void* blob = malloc(size + 1); // We add 1 to fit the null terminator at the end
    error = handle->get_blob(key.c_str(), blob, size);
    if (error != ESP_OK) {
        return Result<shared_ptr<PlugConfig>>::createFailure(format("failed to read blob with key '{}' from nvs: {}", key.c_str(), esp_err_to_name(error)));
    }
    if (blob == nullptr) {
        return Result<shared_ptr<PlugConfig>>::createFailure("failed to read blob from nvs");
    }

    // Add the null terminator to the string
    ((char*)blob)[size] = '\0';

    // Deserialize the plug configuration
    PlugConfig plugConfig = PlugConfig::deserialize(string((char*)blob)); 

    return Result<shared_ptr<PlugConfig>>::createSuccess(make_shared<PlugConfig>(plugConfig));
}

Result<> PlugConfig::writePlugConfig(const string& namespaceValue, const string& key, const PlugConfig& plugConfig) {
    // TODO: Should probably init nvs

    // Get a handle to the Namespace in the NVS
    esp_err_t error;
    unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(
        namespaceValue.c_str(), // Namespace
        NVS_READWRITE,          // Open mode
        &error                  // Error code
    );
    if (error != ESP_OK) {
        return Result<>::createFailure(format("failed to open nvs handle: {}", esp_err_to_name(error)));
    }

    // Serialize the plug configuration
    string serialized = cJSON_PrintUnformatted(serialize().get());

    // Write the serialized plug configuration to the NVS
    error = handle->set_blob(key.c_str(), serialized.c_str(), serialized.length());
    if (error != ESP_OK) {
        return Result<>::createFailure(format("failed to write blob with key '{}' to nvs: {}", key.c_str(), esp_err_to_name(error)));
    }

    // Commit the changes
    error = handle->commit();
    if (error != ESP_OK) {
        return Result<>::createFailure(format("failed to commit changes to nvs: {}", esp_err_to_name(error)));
    }

    return Result<>::createSuccess();
}

unique_ptr<cJSON, void (*)(cJSON *item)> PlugConfig::serialize() {
    // Create a new cJSON object
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

    // Serialize the object
    cJSON_AddItemToObject(root.get(), "bleConfig", bleConfig == nullptr ? cJSON_CreateNull() : bleConfig->serialize().release());
    cJSON_AddItemToObject(root.get(), "acDimmerConfig", acDimmerConfig == nullptr ? cJSON_CreateNull() : acDimmerConfig->serialize().release());
    cJSON_AddItemToObject(root.get(), "wifiConfig", wifiConfig == nullptr ? cJSON_CreateNull() : wifiConfig->serialize().release());
    cJSON_AddItemToObject(root.get(), "mqttConfig", mqttConfig == nullptr ? cJSON_CreateNull() : mqttConfig->serialize().release());

    return root;
}


PlugConfig PlugConfig::deserialize(const string& serialized) {
    // Create a new plug configuration
    PlugConfig plugConfig;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    auto bleConfig = cJSON_GetObjectItem(root.get(), "bleConfig");
    plugConfig.bleConfig = cJSON_IsNull(bleConfig) ? nullptr : make_shared<BleConfig>(BleConfig::deserialize(cJSON_PrintUnformatted(bleConfig)));
    auto acDimmerConfig = cJSON_GetObjectItem(root.get(), "acDimmerConfig");
    plugConfig.acDimmerConfig = cJSON_IsNull(acDimmerConfig) ? nullptr : make_shared<AcDimmerConfig>(AcDimmerConfig::deserialize(cJSON_PrintUnformatted(acDimmerConfig)));
    auto wifiConfig = cJSON_GetObjectItem(root.get(), "wifiConfig");
    plugConfig.wifiConfig = cJSON_IsNull(wifiConfig) ? nullptr : make_shared<WifiConfig>(WifiConfig::deserialize(cJSON_PrintUnformatted(wifiConfig)));
    auto mqttConfig = cJSON_GetObjectItem(root.get(), "mqttConfig");
    plugConfig.mqttConfig = cJSON_IsNull(mqttConfig) ? nullptr : make_shared<MqttConfig>(MqttConfig::deserialize(cJSON_PrintUnformatted(mqttConfig)));

    return plugConfig;
};
