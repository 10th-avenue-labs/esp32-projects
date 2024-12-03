#include <iostream>
#include <string>

#include <memory>
#include <nvs_handle.hpp>

extern "C" {
    #include "nvs_flash.h"
    #include "esp_log.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_system.h"
    #include <string.h>
#include <esp_timer.h>
}

static const char *TAG = "main";

extern "C" void app_main(){
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
                return;
            };
        }
    }

    // Get a handle to the Namespace in the NVS
    ESP_LOGI(TAG, "opening nvs handle");
    esp_err_t error;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(
        "storage",      // Namespace
        NVS_READWRITE,  // Open mode
        &error          // Error code
    );
    ESP_ERROR_CHECK(error);

    // Read the size of the blob from the NVS
    size_t size = 0;
    error = handle->get_item_size(nvs::ItemType::BLOB, "blob_test", size);
    switch (error) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND: {
            ESP_LOGI(TAG, "the blob is not initialized yet!\n");

            // Initialize the blob
            const char* blob = "Hello, World!";
            handle->set_blob("blob_test", blob, strlen(blob));

            // Commit the changes
            handle->commit();
            break;
        }
        default:
            ESP_LOGE(TAG, "error reading blob size: %s", esp_err_to_name(error));
            return;
    }
    ESP_LOGI(TAG, "blob size: %d", size);

    // Read the blob from the NVS
    void* blob = malloc(size + 1); // We add 1 to fit the null terminator at the end
    ESP_ERROR_CHECK(handle->get_blob("blob_test", blob, size));
    if (blob == nullptr) {
        ESP_LOGE(TAG, "failed to read blob from nvs");
        return;
    }

    // Add the null terminator to the string
    ((char*)blob)[size] = '\0';

    // Print the blob
    ESP_LOGI(TAG, "blob: %s", (char*)blob);

    // Get the current time in milliseconds
    uint64_t time = esp_timer_get_time();
    ESP_LOGI(TAG, "time: %llu", time);

    // Create a new string with the time
    std::string newBlob = std::string((char*)blob) + std::to_string(time);
    ESP_LOGI(TAG, "new blob: %s", newBlob.c_str());

    // Write the new blob to the NVS
    ESP_ERROR_CHECK(handle->set_blob("blob_test", newBlob.c_str(), newBlob.size()));
    ESP_ERROR_CHECK(handle->commit());

    // Restart in 10 seconds
    for (int i = 10; i > 0; i--) {
        ESP_LOGI(TAG, "restarting in %d seconds...", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    esp_restart();
}