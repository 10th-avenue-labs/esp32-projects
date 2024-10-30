#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <driver/timer_types_legacy.h>
#include <driver/gptimer_types.h>
#include <driver/gptimer.h>

#define BLINK_GPIO 25  // Set GPIO pin number (usually GPIO2 for built-in LED)

static bool state = 0;
static bool callback_function(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {
    // Toggle the LED
    gpio_set_level(BLINK_GPIO, 1);
    esp_rom_delay_us(50);
    gpio_set_level(BLINK_GPIO, 0);
    state=!state;

    return true;
}

void app_main(void)
{
    // Configure the GPIO pin as output
    printf("Setting output pin\n");
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(BLINK_GPIO, GPIO_PULLUP_ONLY)

    // Create the configuration for a new 1us timer
    printf("Creating timer configuration\n");
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };

    // Create the new timer
    printf("Creating timer\n");
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // Create the callback information
    printf("Creating timer callback information\n");
    gptimer_event_callbacks_t callback = {
        .on_alarm = callback_function
    };

    // Register the callback
    printf("Registering timer callback\n");
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &callback, NULL));

    // Enable the timer
    printf("Enabling timer\n");
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    // Create the configuration for a timer alarm
    printf("Creating alarm config\n");
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 8333, // Timer count value for which an alram event will be triggered
        .flags = {
            .auto_reload_on_alarm = 1 // Whether or not to automatically set the timer count to a specified value when alarms are triggered
        },
        .reload_count = 0 // Value to set the timer clock to when the alarm event triggers (only effective on auto-reload: true)
    };

    // Set the alarm
    printf("Setting alarm\n");
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    // Start the timer
    printf("Starting timer\n");
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    while(1) {
        printf("Hello World\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
