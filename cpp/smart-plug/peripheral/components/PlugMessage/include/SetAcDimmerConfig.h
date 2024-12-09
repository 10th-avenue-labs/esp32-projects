#ifndef SET_AC_DIMMER_CONFIG_H
#define SET_AC_DIMMER_CONFIG_H

#include <string>
#include <cstdint>
#include <cJSON.h>
#include <memory>
#include "IPlugMessageData.h"

using namespace std;

class SetAcDimmerConfig : public IPlugMessageData {
    public:
        uint8_t brightness;

        static unique_ptr<IPlugMessageData> deserialize(const string& serialized);
        string serialize() override { return ""; };
};

#endif // SET_AC_DIMMER_CONFIG_H