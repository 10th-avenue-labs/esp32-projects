#include "PlugConfig.h"

const char* TAG = "PLUG_CONFIG";

tuple<esp_err_t, unique_ptr<PlugConfig>> PlugConfig::readPlugConfig(const string& namespaceValue, const string& key) {
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
                ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", response);
                return {response, nullptr};
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
        ESP_LOGE(TAG, "failed to open nvs handle, error code: %d", error);
        return {error, nullptr};
    }

    // Read the size of the serialized plug configuration from the NVS
    size_t size = 0;
    error = handle->get_item_size(nvs::ItemType::BLOB, key.c_str(), size);
    switch (error) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND: {
            ESP_LOGI(TAG, "the key '%s' is not initialized yet", key.c_str());

            return {ESP_OK, nullptr};
            break;
        }
        default:
            ESP_LOGE(TAG, "error reading blob size: %s", esp_err_to_name(error));
            return {error, nullptr};
    }

    // Read the serialized plug configuration from the NVS
    void* blob = malloc(size + 1); // We add 1 to fit the null terminator at the end
    error = handle->get_blob(key.c_str(), blob, size);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "failed to read blob with key '%s' from nvs, error code: %s", key.c_str(), esp_err_to_name(error));
        return {error, nullptr};
    }
    if (blob == nullptr) {
        ESP_LOGE(TAG, "failed to read blob from nvs");
        return {ESP_FAIL, nullptr};
    }

    // Add the null terminator to the string
    ((char*)blob)[size] = '\0';

    // Deserialize the plug configuration
    PlugConfig plugConfig = PlugConfig::deserialize(string((char*)blob)); 

    return {ESP_OK, make_unique<PlugConfig>(plugConfig)};
}

unique_ptr<cJSON, void (*)(cJSON *item)> PlugConfig::serialize() {
    // Create a new cJSON object
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

    // Serialize the object
    cJSON_AddItemToObject(root.get(), "bleConfig", bleConfig->serialize().release());
    cJSON_AddItemToObject(root.get(), "acDimmerConfig", acDimmerConfig->serialize().release());
    cJSON_AddItemToObject(root.get(), "wifiConfig", wifiConfig->serialize().release());
    cJSON_AddItemToObject(root.get(), "mqttConfig", mqttConfig->serialize().release());

    return root;
}


PlugConfig PlugConfig::deserialize(const string& serialized) {
    // Create a new plug configuration
    PlugConfig plugConfig;

    // Parse the serialized string
    unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(serialized.c_str()), cJSON_Delete);

    // Deserialize the object
    plugConfig.bleConfig = make_shared<BleConfig>(BleConfig::deserialize(cJSON_Print(cJSON_GetObjectItem(root.get(), "bleConfig"))));
    plugConfig.acDimmerConfig = make_shared<AcDimmerConfig>(AcDimmerConfig::deserialize(cJSON_Print(cJSON_GetObjectItem(root.get(), "acDimmerConfig"))));
    plugConfig.wifiConfig = make_shared<WifiConfig>(WifiConfig::deserialize(cJSON_Print(cJSON_GetObjectItem(root.get(), "wifiConfig"))));
    plugConfig.mqttConfig = make_shared<MqttConfig>(MqttConfig::deserialize(cJSON_Print(cJSON_GetObjectItem(root.get(), "mqttConfig"))));

    return plugConfig;
};
