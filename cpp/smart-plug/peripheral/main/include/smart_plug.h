#ifndef SMART_PLUG_H
#define SMART_PLUG_H

extern "C" {
#include <cstdint>
}

class SmartPlug
{
public:
    uint8_t brightness;

    SmartPlug(uint8_t zeroCrossPin, uint8_t triacPin) : zeroCrossPin(zeroCrossPin), triacPin(triacPin) {}

    bool init();
private:
    uint8_t zeroCrossPin;
    uint8_t triacPin;
};

#endif // SMART_PLUG_H