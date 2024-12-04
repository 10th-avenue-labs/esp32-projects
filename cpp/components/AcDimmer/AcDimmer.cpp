#include "AcDimmer.h"

static const char *TAG = "AC_DIMMER";

AcDimmer::AcDimmer(uint8_t zcPin, uint8_t psmPin, uint16_t debounceUs, uint16_t offsetLeading, uint16_t offsetFalling, uint8_t brightness):
    zcPin(zcPin),
    psmPin(psmPin),
    debounceUs(debounceUs),
    offsetLeading(offsetLeading),
    offsetFalling(offsetFalling),
    brightness(brightness)
{
    // Initialize the GPIO pins
    ESP_LOGI(TAG, "initializing GPIO pins");
    gpio_set_direction((gpio_num_t) psmPin, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t) zcPin, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t) zcPin, GPIO_PULLUP_ONLY);

    // Initialize the pin interrupt for the zero cross pin
    ESP_LOGI(TAG, "initializing zero cross interrupt");

    // Set the interrupt type to positive edge
    ESP_ERROR_CHECK(gpio_set_intr_type((gpio_num_t) zcPin, GPIO_INTR_POSEDGE));

    // Install the ISR service configured for edge-triggered interrupts
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));

    // Register the ISR
    ESP_ERROR_CHECK(gpio_isr_handler_add((gpio_num_t) zcPin, zeroCrossInterruptHandler, this));
}

void AcDimmer::setBrightness(uint8_t brightness) {
    this->brightness = brightness;
}

uint8_t AcDimmer::getBrightness() {
    return this->brightness;
}

void AcDimmer::zeroCrossInterruptHandler(void* arg) {
    // Convert the arguments to an AcDimmer object
    AcDimmer* acDimmer = (AcDimmer*) arg;

    // Get the current time
    struct timeval timeval;
    gettimeofday(&timeval, NULL);
    uint64_t currentTime = timeval.tv_sec * 1000 * 1000 + timeval.tv_usec;

    // Check if the last interrupt time has been established
    if (
        acDimmer->lastInterruptTime == 0
    ) {
        // Set the last interrupt time
        acDimmer->lastInterruptTime = currentTime;
        return;
    }

    // Debounce the interrupt
    if (currentTime < (acDimmer->lastInterruptTime + acDimmer->debounceUs)) {
        return;
    }

    // Calculate the phase shift modulation time
    uint64_t totalTime = currentTime - acDimmer->lastInterruptTime - acDimmer->offsetLeading - acDimmer->offsetFalling;

    // Set the last interrupt time
    acDimmer->lastInterruptTime = currentTime;

    // Stop the timer
    acDimmer->timer.stop();

    // Reset the timer
    acDimmer->timer.reset();

    // Calculate the delay after the interrupt for when the PSM pin should pulse
    uint16_t psmTime = acDimmer->offsetLeading + totalTime - totalTime * acDimmer->brightness / 255;

    // Set the timer alarm
    acDimmer->timer.setAlarm(psmTime, false, [](Timer* timer, void* args) {
        // Convert the arguments to an AcDimmer object
        AcDimmer* acDimmer = (AcDimmer*) args;

        // Send a pulse to the phase shift modulation pin
        busyPulse(acDimmer->psmPin, 10);
    }, acDimmer);

    // Start the timer
    acDimmer->timer.start();
}

void AcDimmer::busyPulse(uint8_t pin, uint16_t pulseWidthUs) {
    gpio_set_level((gpio_num_t) pin, 1);
    esp_rom_delay_us(pulseWidthUs);
    gpio_set_level((gpio_num_t) pin, 0);
}