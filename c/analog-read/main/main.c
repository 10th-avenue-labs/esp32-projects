#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#define ADC_PIN ADC_CHANNEL_4 // GPIO pin 32

void app_main(void)
{
    // Set the pin as an input
    gpio_set_direction(32, GPIO_MODE_OUTPUT);

    // Create a ADC one-shot handle
    adc_oneshot_unit_handle_t adc_handle;

    // Create an ADC config
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };

    // Create a handle to and ADC on-shot
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // Create an ADC one-shot channel config
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12, // Read to the max voltage (I think, not really sure here but I think this value has to do with the maximum readable voltage)
    };

    // Add the ADC one-shot channel
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_PIN, &config));

    // Read the ADC
    int adc_raw = 0;

    while(1) {
        // Read the channel
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_PIN, &adc_raw));

        // Print the result
        printf("Raw: %d\n", adc_raw);

        // Delay for a second before reading again
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Recycle the handle when done
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
}