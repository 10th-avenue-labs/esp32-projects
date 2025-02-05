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
     * @param skipFirstWait Whether or not to skip the first wait
     * @return std::function<int(int, uint64_t, uint64_t)>
     */
    static std::function<int(int, uint64_t, uint64_t)> ConstantTime(int milliseconds, bool skipFirstWait = false)
    {
        return [milliseconds, skipFirstWait](int waitNumber, uint64_t currentTime, uint64_t timeAtLastWaitCompletion)
        {
            // Check if this is the first wait
            if (waitNumber == 0 && skipFirstWait)
            {
                return 0;
            }

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
        return [milliseconds](int waitNumber, uint64_t currentTime, uint64_t timeAtLastWaitCompletion)
        {
            // Check if this is the first wait
            if (waitNumber == 0)
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
     * @brief Wait for an exponential amount of time sense the completion of the last wait. The wait time is calculated as follows:
     * multiplier * base ^ waitNumber and then made relative to the last wait completion time
     *
     * @param base The base number to use for the exponential growth
     * @param multiplier The multiplier to use for the exponential growth
     * @return std::function<int(int, uint64_t, uint64_t)>
     */
    static std::function<int(int, uint64_t, uint64_t)> ExponentialTime(int base, int multiplier)
    {
        return [base, multiplier](int waitNumber, uint64_t currentTime, uint64_t timeAtLastWaitCompletion)
        {
            // Check if this is the first wait
            if (waitNumber == 0)
            {
                return 0;
            }

            // Calculate the absolute, un-adjusted wait time
            uint64_t absoluteWaitMs = (uint64_t)(multiplier * pow(base, waitNumber));

            // Caluculate the time we should be waiting until
            uint64_t waitUntil = timeAtLastWaitCompletion + absoluteWaitMs;

            // Calculate the number of milliseconds to wait
            uint64_t waitMs = currentTime < waitUntil ? waitUntil - currentTime : 0;

            return (int)waitMs;
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
     * @brief Set a maximum time to wait for each wait. If the maximum wait time is reached, the waiter will wait for the maximum time for each subsequent wait until reset.
     *
     * @param maxWaitMs The maximum number of milliseconds to wait for each wait
     */
    void setMaxWaitMs(int maxWaitMs)
    {
        this->maxWaitMs = maxWaitMs;
    }

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

        // Get the current time
        uint64_t currentTime = getTimeMs();

        // Calculate the wait time from the wait function
        int waitMs = waitTimeFunction(waitNumber, currentTime, timeAtLastWaitCompletion);

        // Calculate the maximum allowed wait time
        int maxWaitTime = timeAtLastWaitCompletion + maxWaitMs - currentTime;
        maxWaitTime = maxWaitTime > 0 ? maxWaitTime : 0;

        // Check if the maximum wait time has been reached
        if (maxWaitMsReached || (maxWaitMs != -1 && waitMs > maxWaitTime))
        {
            ESP_LOGI(WAITER_TAG, "max wait time reached, waiting for %d ms", maxWaitTime);
            waitMs = maxWaitTime;
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
     * @brief Get the current wait number
     *
     * @return int The current wait number
     */
    int getWaitNumber()
    {
        return waitNumber;
    }

    /**
     * @brief Reset the waiter
     *
     * @return Result<> A result object indicating success or failure. This fails if the waiter is currently waiting
     */
    Result<> reset()
    {
        // Verify we are not currently waiting
        bool expected = false;
        if (!waiting.compare_exchange_strong(expected, true))
        {
            return Result<>::createFailure("Attempted to reset while already waiting");
        }

        waitNumber = 0;
        timeAtLastWaitCompletion = 0;
        maxWaitMsReached = false;

        // Reset the waiting flag
        waiting.store(false);

        return Result<>::createSuccess();
    };

    /**
     * @brief Get the current time in milliseconds
     *
     * @return uint64_t The current time in milliseconds
     */
    static uint64_t getTimeMs()
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
    uint64_t timeAtLastWaitCompletion = 0;                                                                   // The time in milliseconds when the last wait finished, 0 if no wait has been called
    function<int(int waitNumber, uint64_t currentTime, uint64_t timeAtLastWaitCompletion)> waitTimeFunction; // Function to calculate the wait time in ms
    int maxWaitMs = -1;                                                                                      // The maximum number of milliseconds to wait
    bool maxWaitMsReached = false;                                                                           // Whether or not the maximum wait time for individual waits has been reached. This helps avoid integer overflows
};

#endif // WAITER_H