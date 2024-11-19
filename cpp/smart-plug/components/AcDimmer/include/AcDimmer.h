#include "Timer.h"

extern "C" {
    #include "driver/gpio.h"
    #include <esp_log.h>
    #include <cstdint>
    #include <sys/time.h>
}

class AcDimmer {
    public:
        /**
         * @brief Construct a new Ac Dimmer object
         * 
         * @param zcPin The zero crossing pin
         * @param psmPin The phase shift modulation pin
         * @param debounceUs The debounce time in microseconds between zero crossing events
         * @param offsetLeading The offset in microseconds to lead the zero crossing event
         * @param offsetFalling The offset in microseconds to fall the zero crossing event
         */
        AcDimmer(uint8_t zcPin, uint8_t psmPin, uint16_t debounceUs, uint16_t offsetLeading, uint16_t offsetFalling);

        /**
         * @brief Set the brightness of the dimmer
         * 
         * @param brightness The brightness of the dimmer (0-255)
         */
        void setBrightness(uint8_t brightness);

        /**
         * @brief Get the brightness of the dimmer
         * 
         * @return uint8_t The brightness of the dimmer (0-255)
         */
        uint8_t getBrightness();
    private:
        uint8_t psmPin;                 // The phase shift modulation pin
        uint8_t zcPin;                  // The zero crossing pin
        uint64_t lastInterruptTime = 0; // The time sense the last interrupt measured in microseconds
        uint16_t debounceUs;            // The debounce time in microseconds between zero crossing events
        uint16_t offsetLeading;         // The offset in microseconds to lead the zero crossing event
        uint16_t offsetFalling;         // The offset in microseconds to fall the zero crossing event
        uint8_t brightness;             // The brightness of the dimmer (0-255)
        Timer timer;                    // The timer used to control the phase shift modulation pin

        /**
         * @brief The zero crossing interrupt handler
         * 
         * @param arg The AcDimmer object
         */
        static void zeroCrossInterruptHandler(void* arg);

        /**
         * @brief Send a busy pulse to a pin
         * 
         * @param pin The pin to send the pulse to
         * @param durationUs The duration of the pulse in microseconds
         */
        static void busyPulse(uint8_t pin, uint16_t durationUs);
};