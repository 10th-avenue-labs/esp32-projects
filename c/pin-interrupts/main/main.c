#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <time.h>
#include <sys/time.h>

#define ZC_GPIO 25  // Set GPIO pin number

int lastTime = 0;
int debounce = 1000;
static int interrupt_set = false;
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

    interrupt_set = true;
}

void app_main(void)
{
    // Set the pin directions
    gpio_set_direction(ZC_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ZC_GPIO, GPIO_PULLUP_ONLY);

    // Set the interrupt type
    ESP_ERROR_CHECK(gpio_set_intr_type(ZC_GPIO, GPIO_INTR_POSEDGE));

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));

    // Register the ISR
    ESP_ERROR_CHECK(gpio_isr_handler_add(ZC_GPIO, isr, NULL));

    int counter = 0;
    while(1) {
        // Delay for a second before reading again
        vTaskDelay(pdMS_TO_TICKS(10));
        if (!interrupt_set) continue;
        interrupt_set = false;
        counter++;

        printf("Interrupt %d\n", counter);
    }
}