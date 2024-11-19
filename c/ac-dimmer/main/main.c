#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <time.h>
#include <sys/time.h>
#include "esp_timer.h"
#include <driver/gptimer_types.h>
#include <driver/gptimer.h>


#define ZC_GPIO 32  // Set ZC pin
#define PSM_GPIO 25  // Set PSM pin

gptimer_handle_t gptimer = NULL;


static void busyPulse(gpio_num_t gpio_num, uint32_t us_delay) {
    gpio_set_level(gpio_num, 1);
    esp_rom_delay_us(us_delay);
    gpio_set_level(gpio_num, 0);
}

static bool alarm_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {
    // Stop and reset the timer
    gptimer_stop(gptimer);
    gptimer_set_raw_count(gptimer, 0);

    // Busy pulse the PSM pin for 50us
    busyPulse(PSM_GPIO, 50);

    return true;
}

int lastTime = 0;
int debounce = 1000;
static void isr(void* arg)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int time = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    if (time < lastTime + debounce) {
        lastTime = time;
        return;
    }

    lastTime = time;

    // Start the timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

void app_main(void)
{
    // Set the pin directions
    gpio_set_direction(PSM_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(ZC_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ZC_GPIO, GPIO_PULLUP_ONLY);


    //// Timer stuff


    // Create the configuration for a new 1us timer
    printf("Creating timer configuration\n");
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
        .on_alarm = alarm_handler
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
        .alarm_count = 4000, // Timer count value for which an alram event will be triggered
    };

    // Set the alarm
    printf("Setting alarm\n");
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    //// Pin ISR stuff

    // Set the interrupt type
    ESP_ERROR_CHECK(gpio_set_intr_type(ZC_GPIO, GPIO_INTR_POSEDGE));

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));

    // Register the ISR
    ESP_ERROR_CHECK(gpio_isr_handler_add(ZC_GPIO, isr, NULL));

    int startVal = 4000;
    int val = startVal;
    while(1) {
        val += 10;
        if (val > 8000) {
            val = startVal;
        }
        gptimer_alarm_config_t alarm_config = {
            .alarm_count = val, // alarm in next 1s
        };
        gptimer_set_alarm_action(gptimer, &alarm_config);

        printf("Hello World %d\n", val);

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}