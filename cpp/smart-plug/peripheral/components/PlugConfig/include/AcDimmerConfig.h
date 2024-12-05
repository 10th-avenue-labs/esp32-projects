#ifndef AC_DIMMER_CONFIG_H
#define AC_DIMMER_CONFIG_H

#include <memory>
#include "cJSON.h"

using namespace std;

class AcDimmerConfig {
    public:
        int zcPin;
        int psmPin;
        int debounceUs;
        int offsetLeading;
        int offsetFalling;
        int brightness;

        unique_ptr<cJSON, void (*)(cJSON *item)> serialize();
        static AcDimmerConfig deserialize(const string& serialized);
};

#endif // AC_DIMMER_CONFIG_H