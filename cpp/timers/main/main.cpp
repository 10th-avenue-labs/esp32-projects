#include "Timer.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

static const char *TAG = "timer main";

void alarmHandler(Timer* timer, void* args) {
    ESP_EARLY_LOGI(TAG, "alarm");
    int* triggered = (int*) args;
    (*triggered)++;
    ESP_EARLY_LOGI(TAG, "triggered address: %p", triggered);

    // Set the next alarm
    timer->setAlarm(timer->getAlarmTrigger() + 1000000, false, alarmHandler, args);
}

extern "C" void app_main(){
    // Create a new timer
    ESP_LOGI(TAG, "creating a new timer");
    Timer timer;

    int triggered = 0;
    ESP_LOGI(TAG, "triggered address: %p", &triggered);

    // Set the alarm
    ESP_LOGI(TAG, "setting the alarm");
    timer.setAlarm(1000000, false, alarmHandler, &triggered);

    // Start the timer
    ESP_LOGI(TAG, "starting the timer");
    timer.start();

    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "timer count: %" PRId64, timer.getCount());
        ESP_LOGI(TAG, "timer trigger: %" PRId64, timer.getAlarmTrigger());
        ESP_LOGI(TAG, "timer count: %d", triggered);

        if (triggered >= 5) {
            ESP_LOGI(TAG, "stopping the timer");
            timer.stop();
            break;
        }
    }
}