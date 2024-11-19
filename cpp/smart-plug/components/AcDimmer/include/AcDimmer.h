#include "Timer.h"

extern "C" {
    #include "driver/gpio.h"
    #include <esp_log.h>
    #include <cstdint>
}

class AcDimmer {
    public:


        /**
         * @brief Construct a new Ac Dimmer object
         * 
         * @param zcPin The zero crossing pin
         * @param psmPin The phase shift modulation pin
         * @param debounceUs The debounce time in microseconds between zero crossing events
         */
        AcDimmer(uint8_t zcPin, uint8_t psmPin, uint16_t debounceUs, uint16_t offsetLeading, uint16_t offsetFalling);

        void setBrightness(uint8_t brightness);

        uint8_t getBrightness();
    private:
        uint8_t psmPin;
        uint8_t zcPin;
        uint64_t lastInterruptTime = 0;
        uint16_t debounceUs;
        uint16_t offsetLeading;
        uint16_t offsetFalling;
        uint8_t brightness;
        Timer timer;

        static void zeroCrossInterruptHandler(void* arg);
        static void busyPulse(uint8_t pin, uint16_t durationUs);
};