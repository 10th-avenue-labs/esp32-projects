#include <iostream>
#include <string>

#include "Waiter.h"
#include "esp_log.h"
#include "esp_event.h"

extern "C" void app_main()
{
    Waiter w{WaitFunctions::LinearMs(5000)};

    w.getTimeMs();

    while (true)
    {
        // ESP_LOGI("main", "Current time: %llu", w.getTimeMs());
        w.wait();
    }
}