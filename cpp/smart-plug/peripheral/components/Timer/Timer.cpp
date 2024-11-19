#include "Timer.h"

static const char *TAG = "TIMER";

///////////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
///////////////////////////////////////////////////////////////////////////////

Timer::Timer(){
    // Create the configuration for a new timer
    ESP_LOGI(TAG, "creating a new timer config");
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // Minimum resolution is greater than 1000. Because of this we will create a micro-second resolution timer
    };

    // Create the new timer
    ESP_LOGI(TAG, "creating a new timer");
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // Create the callback information
    ESP_LOGI(TAG, "creating the callback information");
    gptimer_event_callbacks_t callbackConfig = {
        .on_alarm = [](gptimer_handle_t _, const gptimer_alarm_event_data_t *__, void *args) -> bool {
            // Convert the arguments to a timer
            Timer* timer = (Timer*) args;

            // Check if a callback is registered
            std::function<void(Timer*, void*)> callback = timer->callback;
            if (callback != nullptr) {
                // Execute the callback
                callback(timer, timer->callbackArgs);
            }

            return true;
        },
    };

    // Register the callback
    ESP_LOGI(TAG, "registering the callback");
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &callbackConfig, this));

    // Enable the timer
    ESP_LOGI(TAG, "enabling the timer");
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
}

Timer::~Timer(){
    // Stop the timer
    stop();

    // Disable the timer
    ESP_LOGI(TAG, "disabling timer");
    ESP_ERROR_CHECK(gptimer_disable(gptimer));

    // Delete the timer
    ESP_LOGI(TAG, "deleting the timer");
    ESP_ERROR_CHECK(gptimer_del_timer(gptimer));
}

///////////////////////////////////////////////////////////////////////////////
// Start/stop/reset
///////////////////////////////////////////////////////////////////////////////

void Timer::start()
{
    // Start the timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    // Set the running flag
    running = true;
}

void Timer::stop()
{
    // Check if the timer is running
    if (!running) {
        // The timer is not running
        return;
    }

    // Stop the timer
    ESP_ERROR_CHECK(gptimer_stop(gptimer));

    // Reset the running flag
    running = false;
}

void Timer::reset()
{
    // Reset the timer
    ESP_ERROR_CHECK(gptimer_set_raw_count(gptimer, 0));
}

///////////////////////////////////////////////////////////////////////////////
// Alarm
///////////////////////////////////////////////////////////////////////////////

void Timer::setAlarm(uint64_t triggerCount, bool periodic, std::function<void(Timer*, void*)> callback, void* args)
{
    // Set the alarm trigger
    alarmTrigger = triggerCount;

    // Create the configuration for a timer alarm
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = triggerCount, // Timer count value for which an alram event will be triggered
        .reload_count = 0, // Value to set the timer clock to when the alarm event triggers (only effective on auto-reload: true)
        .flags = {
            .auto_reload_on_alarm = periodic // Whether or not to automatically set the timer count to a specified value when alarms are triggered
        },
    };

    // Set the alarm
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    // Register the callback
    this->callback = callback;

    // Register the callback arguments
    this->callbackArgs = args;
}

uint64_t Timer::getAlarmTrigger() {
    // Get the current alarm trigger value
    return alarmTrigger;
}

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

uint64_t Timer::getCount() {
    // Get the current count of the timer
    uint64_t count;
    ESP_ERROR_CHECK(gptimer_get_raw_count(gptimer, &count));
    return count;
}

bool Timer::isRunning() {
    // Return the running flag
    return running;
}
