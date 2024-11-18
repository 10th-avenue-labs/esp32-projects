#include "include/smart_plug.h"

#include "driver/gpio.h"
#include <esp_log.h>
#include "esp_event.h"

static const char *TAG = "SMART_PLUG";

bool SmartPlug::init() {
    // Initialize the GPIO pins
    ESP_LOGI(TAG, "initializing GPIO pins");
    gpio_set_direction((gpio_num_t) triacPin, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t) zeroCrossPin, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t) zeroCrossPin, GPIO_PULLUP_ONLY);

    // 


    return true;
}