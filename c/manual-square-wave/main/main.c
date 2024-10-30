#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define BLINK_GPIO 27  // Set GPIO pin number

void app_main(void)
{
    // Configure the GPIO pin as output
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while (1) {
        // Turn the LED on
        gpio_set_level(BLINK_GPIO, 1);
        esp_rom_delay_us(50); // Busy-wait for a number of microseconds (blocking)
        // vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay of 1 second (non-blocking)

        // Turn the LED off
        gpio_set_level(BLINK_GPIO, 0);
        esp_rom_delay_us(50);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
