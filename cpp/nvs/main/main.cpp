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

    // Read a vale from the NVS
    int32_t restartCounter = 0; // value will default to 0, if not set yet in NVS
    error = handle->get_item("restart_counter", restartCounter);
    switch (error) {
        case ESP_OK:
            ESP_LOGI(TAG, "restart counter = %ld", restartCounter);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG, "the value is not initialized yet!\n");
            break;
        default :
            ESP_LOGE(TAG, "error (%s) reading!\n", esp_err_to_name(error));
    }

    // Increment the restart counter in the NVS
    ESP_LOGI(TAG, "incrementing restart counter");
    restartCounter++;
    error = handle->set_item("restart_counter", restartCounter);
    ESP_ERROR_CHECK(error);

    // Commit the changes to the NVS
    ESP_LOGI(TAG, "committing changes");
    error = handle->commit();
    ESP_ERROR_CHECK(error);

    // Restart module
    for (int i = 10; i >= 0; i--) {
        ESP_LOGI(TAG, "restarting in %d seconds...", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "restarting now.\n");
    fflush(stdout);
    esp_restart();
}