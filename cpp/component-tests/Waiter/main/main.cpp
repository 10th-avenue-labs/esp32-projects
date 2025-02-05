#include <iostream>
#include <string>

#include "Waiter.h"
#include "esp_log.h"
#include "esp_event.h"

extern "C" void app_main()
{
    // Constant wait (tested and works)
    Waiter w{WaitFunctions::ConstantTime(5000)};
    // Linear wait (tested and works)
    // Waiter w{WaitFunctions::LinearMs(5000)};
    // Exponential wait (tested and works)
    // Waiter w{WaitFunctions::ExponentialTime(2, 1000)};
    w.setMaxWaitMs(1000 * 6);

    while (true)
    {
        Result<> res = w.wait();
        if (!res.isSuccess())
        {
            ESP_LOGE("main", "failed to wait: %s", res.getError().c_str());
        }

        if (w.getWaitNumber() > 7)
        {
            ESP_LOGI("main", "resetting waiter");
            w.reset();
        }

        // Calculate a random number between 1 and 10
        int random = rand() % 10 + 1;

        ESP_LOGI("main", "waiting for %d seconds from main to simulate logic", random);

        // Delay for a random amount of time
        vTaskDelay(random * 1000 / portTICK_PERIOD_MS);
    }
}