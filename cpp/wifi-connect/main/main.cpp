#include <string.h>
#include <inttypes.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "include/wifi_service.h"


static const char *TAG = "wifi station";

extern "C" void app_main(void)
{
    WifiService wifiService = WifiService();

    // Log the WiFi mode
    ESP_LOGI(TAG, "initializing wifi service");
    wifiService.init(
        "denhac",
        "denhac rules"
    );

    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
