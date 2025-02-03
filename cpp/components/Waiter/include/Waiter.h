#ifndef WAITER_H
#define WAITER_H

#include "Result.h"
#include <atomic>
#include "esp_log.h"
#include "esp_event.h"
#include <functional>

using namespace std;

static const char *WAITER_TAG = "WAITER";

struct WaitFunctions
{
    /**
     * @brief Wait a linear number of milliseconds sense the last wait, this is relative to the last wait time and does not wait a fixed amount of time every time.
     *
     * @param milliseconds The number of milliseconds to wait sense the last wait
     * @return std::function<int(int, int, int)>
     */
    static std::function<int(int, uint64_t, uint64_t)> LinearMs(int milliseconds)
    {
        return [milliseconds](int _, uint64_t currentTime, uint64_t timeAtLastWait)
        {
            // TODO: Fix this logic, it's all kinds of wrong

            // Check if this is the first wait
            if (timeAtLastWait == -1)
            {
                return 0;
            }

            ESP_LOGI(WAITER_TAG, "wait number: %d, time at last wait: %llu, current time: %llu, time sinse last wait %llu",
                     _,
                     timeAtLastWait,
                     currentTime,
                     currentTime - timeAtLastWait);

            // Caluculate the time we should be waiting until
            uint64_t waitUntil = timeAtLastWait + (uint64_t)milliseconds;

            // Calculate the number of milliseconds to wait
            uint64_t waitMs = currentTime < waitUntil ? waitUntil - currentTime : 0;
            ESP_LOGI(WAITER_TAG, "we should wait until: %llu which is %llu ms from now", waitUntil, waitMs);
            return (int)waitMs;
            // return msToWait > 0 ? msToWait : 0;

            // // Calculate the number of milliseconds since the last wait
            // int msSinseLastWait = currentTime - timeAtLastWait;
            // return msSinseLastWait < milliseconds ? milliseconds - msSinseLastWait : 0;
        };
    }

    // Another example with exponential growth base
    static std::function<int(int, int, int)> ExponentialBase(int base)
    {
        return [base](int waitNumber, int currentTime, int timeAtLastWait)
        {
            return timeAtLastWait + base * (1 << waitNumber); // Exponential growth
        };
    }
};

class Waiter
{
public:
    Waiter(function<int(int waitNumber, uint64_t currentTime, uint64_t timeAtLastWait)> waitTimeFunction) : waitTimeFunction(waitTimeFunction) {};
    Waiter(int waitTimeMs) : waitTimeFunction(WaitFunctions::LinearMs(waitTimeMs)) {};

    Result<> wait()
    {
        // Ensure we are not already waiting
        bool expected = false;
        if (!waiting.compare_exchange_strong(expected, true))
        {
            ESP_LOGE(WAITER_TAG, "attempted to wait while already waiting");
            return Result<>::createFailure("Attempted to wait while already waiting");
        }

        // Get the current time
        uint64_t currentTime = getTimeMs();

        // Calculate the number of milliseconds to wait
        int waitMs = waitTimeFunction(waitNumber, currentTime, timeAtLastWait);

        // Wait for the specified amount of time
        // ESP_LOGI(WAITER_TAG, "waiting for %d milliseconds on attempt %d, with a current time of %llu and a last wait time of %d", waitMs, waitNumber, currentTime, timeAtLastWait);
        if (waitMs > 0)
        {
            vTaskDelay(waitMs / portTICK_PERIOD_MS);
        }

        // Update the wait number and time
        waitNumber++;
        timeAtLastWait = currentTime + (uint64_t)waitMs;

        // Reset the waiting flag
        waiting.store(false);

        return Result<>::createSuccess();
    };

    /**
     * @brief Reset the waiter
     *
     */
    void reset()
    {
        waitNumber = 0;
        timeAtLastWait = 0;
    };

    /**
     * @brief Get the current time in milliseconds
     *
     * @return uint64_t The current time in milliseconds
     */
    uint64_t getTimeMs()
    {
        // Get the current time
        struct timeval timeval;
        gettimeofday(&timeval, NULL);
        uint64_t currentTime = timeval.tv_sec * 1000 + timeval.tv_usec / 1000;

        return currentTime;
    }

private:
    atomic<bool> waiting = atomic<bool>{false};                                                  // Whether or not we are currently waiting
    int waitNumber = 0;                                                                          // Number of waits called sinse inception or last reset
    int timeAtLastWait = -1;                                                                     // The time in milliseconds at the last wait, -1 if no wait has been called
    function<int(int waitNumber, uint64_t currentTime, uint64_t lastWaitTime)> waitTimeFunction; // Function to calculate the wait time in ms
};

#endif // WAITER_H