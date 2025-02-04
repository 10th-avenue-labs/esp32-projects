#include <iostream>
#include <string>

#include "Waiter.h"
#include "esp_log.h"
#include "esp_event.h"

extern "C" void app_main()
{
    Waiter w{WaitFunctions::LinearMs(5000)};

    w.getTimeMs();

    ESP_LOGI("main", "Starting main loop");
    ESP_LOGI("main", "Tick period: %lu", portTICK_PERIOD_MS);

    while (true)
    {
        ESP_LOGI("main", "Current time: %llu", w.getTimeMs());
        Result<> res = w.wait();
        if (!res.isSuccess())
        {
            ESP_LOGE("main", "failed to wait: %s", res.getError().c_str());
        }
        else
        {
            ESP_LOGI("main", "waited successfully");
        }
    }
}