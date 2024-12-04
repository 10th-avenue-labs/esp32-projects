#ifndef TIMER_H
#define TIMER_H

#include <functional>

#include <inttypes.h>
#include <driver/gptimer.h>
#include <esp_log.h>

class Timer {
    public:
        ////////////////////////////////////////////////////////////////////////
        // Constructor/Destructor
        ////////////////////////////////////////////////////////////////////////

        /**
         * @brief Construct a new Timer object
         * 
         */
        Timer();

        /**
         * @brief Destroy the Timer object
         * 
         */
        ~Timer();

        ////////////////////////////////////////////////////////////////////////
        // Start/stop/reset
        ////////////////////////////////////////////////////////////////////////

        /**
         * @brief Start the timer
         * 
         * This can be run from within an ISR context
         */
        void start();

        /**
         * @brief Stop the timer
         * 
         * This can be run from within an ISR context
         */
        void stop();

        /**
         * @brief Reset the timer
         * 
         * This can be run from within an ISR context
         */
        void reset();

        ////////////////////////////////////////////////////////////////////////
        // Alarm
        ////////////////////////////////////////////////////////////////////////

        /**
         * @brief Set the alarm
         * 
         * This can be run from within an ISR context
         * 
         * @param triggerCount The count (in microseconds) at which the alarm will trigger
         * @param periodic True if the alarm should be periodic, false otherwise. If true, the alarm will trigger every triggerCount microseconds
         * @param callback The callback function to call when the alarm triggers
         * @param args The arguments to pass to the callback function
         */
        void setAlarm(uint64_t triggerCount, bool periodic, std::function<void(Timer*, void*)> callback, void* args);

        /**
         * @brief Get the alarm trigger count in microseconds
         * 
         * @return The alarm trigger count in microseconds
         */
        uint64_t getAlarmTrigger();

        ////////////////////////////////////////////////////////////////////////
        // Getters
        ////////////////////////////////////////////////////////////////////////

        /**
         * @brief Get the current count of the timer in microseconds
         * 
         * @return The count of the timer
         */
        uint64_t getCount();

        /**
         * @brief Check if the timer is running
         * 
         * @return True if the timer is running, false otherwise
         */
        bool isRunning();
    private:
        gptimer_handle_t gptimer;
        uint64_t alarmTrigger;
        std::function<void(Timer*, void*)> callback;
        void* callbackArgs;
        bool running = false;
};

#endif // TIMER_H