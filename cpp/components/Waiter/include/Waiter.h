#ifndef WAITER_H
#define WAITER_H

#include <cmath>
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
     * @brief Wait for a constant amount of time in milliseconds, regardless of when the last wait was and the number of waits that have been called
     *
     * Ex: if 5000 is passed in, the waiter will wait for 5000 ms every time
     *
     * @param milliseconds The number of milliseconds to wait
     * @return std::function<int(int, uint64_t, uint64_t)>
     */
    static std::function<int(int, uint64_t, uint64_t)> ConstantTime(int milliseconds)
    {
        return [milliseconds](int waitNumber, uint64_t currentTime, uint64_t timeAtLastWaitCompletion)
        {
            return milliseconds;
        };
    }

    /**
     * @brief Wait a linear number of milliseconds sense the last wait, this is relative to the last wait time and does not wait a fixed amount of time every time
     *
     * @param milliseconds The number of milliseconds to wait sense the last wait completed
     * @return std::function<int(int, uint64_t, uint64_t)>
     */
    static std::function<int(int, uint64_t, uint64_t)> LinearMs(int milliseconds)
    {
        return [milliseconds](int _, uint64_t currentTime, uint64_t timeAtLastWaitCompletion)
        {
            // Check if this is the first wait
            if (timeAtLastWaitCompletion == -1)
            {
                return 0;
            }

            // Caluculate the time we should be waiting until
            uint64_t waitUntil = timeAtLastWaitCompletion + (uint64_t)milliseconds;

            // Calculate the number of milliseconds to wait
            uint64_t waitMs = currentTime < waitUntil ? waitUntil - currentTime : 0;
            return (int)waitMs;
        };
    }

    /**
     * @brief Wait for an exponential amount of time sense the completion of the last wait
     *
     * @param base The base number to use for the exponential growth
     * @return std::function<int(int, uint64_t, uint64_t)>
     */
    static std::function<int(int, uint64_t, uint64_t)> ExponentialTime(int base)
    {
        return [base](int waitNumber, uint64_t currentTime, uint64_t timeAtLastWaitCompletion)
        {
            // Check if this is the first wait
            if (timeAtLastWaitCompletion == -1)
            {
                return 0;
            }

            // Calculate the wait duration for this wait
            uint64_t waitMs = (uint64_t)pow(base, waitNumber);

            // Caluculate the time we should be waiting until
            uint64_t waitUntil = timeAtLastWaitCompletion + (uint64_t)milliseconds;

            return 0;
        };
    }
};

class Waiter
{
public:
    /**
     * @brief Construct a new Waiter object with a custom wait time function
     *
     * @param waitTimeFunction The function to calculate the wait time in milliseconds for each wait
     */
    Waiter(function<int(int waitNumber, uint64_t currentTime, uint64_t timeAtLastWait)> waitTimeFunction) : waitTimeFunction(waitTimeFunction) {};

    /**
     * @brief Construct a new Waiter object with a fixed wait time in milliseconds
     *
     * @param waitTimeMs The number of milliseconds to wait for each wait
     */
    Waiter(int waitTimeMs) : waitTimeFunction(WaitFunctions::LinearMs(waitTimeMs)) {};

    /**
     * @brief Wait for an amount of time defined by the waiting function upon initialization
     *
     * @return Result<> A result object indicating success or failure. This fails if the waiter is already waiting
     */
    Result<> wait()
    {
        // Ensure we are not already waiting
        bool expected = false;
        if (!waiting.compare_exchange_strong(expected, true))
        {
            ESP_LOGE(WAITER_TAG, "attempted to wait while already waiting");
            return Result<>::createFailure("Attempted to wait while already waiting");
        }

        // Check if we have reached the maximum wait time
        if (maxWaitMsReached)
        {
            // Wait for the maximum wait time
            ESP_LOGI(WAITER_TAG, "waiting for max wait time of %d ms", maxWaitMs);
            vTaskDelay(maxWaitMs / portTICK_PERIOD_MS);

            // Reset the waiting flag
            waiting.store(false);

            return Result<>::createSuccess();
        }

        // Get the current time
        uint64_t currentTime = getTimeMs();

        // Calculate the number of milliseconds to wait
        int waitMs = waitTimeFunction(waitNumber, currentTime, timeAtLastWaitCompletion);

        // Check if we have reached the maximum wait time
        if (maxWaitMs > 0 && waitMs > maxWaitMs)
        {
            waitMs = maxWaitMs;
            maxWaitMsReached = true;
        }

        // Wait for the specified amount of time
        if (waitMs > 0)
        {
            ESP_LOGI(WAITER_TAG, "waiting for %d ms", waitMs);
            vTaskDelay(waitMs / portTICK_PERIOD_MS);
        }

        // Update the wait number and time
        waitNumber++;
        timeAtLastWaitCompletion = currentTime + (uint64_t)waitMs;

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
        timeAtLastWaitCompletion = -1;
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
    atomic<bool> waiting = atomic<bool>{false};                                                              // Whether or not we are currently waiting
    int waitNumber = 0;                                                                                      // Number of waits called sinse inception or last reset
    int timeAtLastWaitCompletion = -1;                                                                       // The time in milliseconds when the last wait finished, -1 if no wait has been called
    function<int(int waitNumber, uint64_t currentTime, uint64_t timeAtLastWaitCompletion)> waitTimeFunction; // Function to calculate the wait time in ms
    int maxWaitMs = -1;                                                                                      // The maximum number of milliseconds to wait
    bool maxWaitMsReached = false;                                                                           // Whether or not the maximum wait time for individual waits has been reached
};

#endif // WAITER_H