#ifndef SET_MQTT_CONFIG_H
#define SET_MQTT_CONFIG_H

#include <string>
#include <cJSON.h>
#include <memory>
#include "IPlugMessageData.h"

using namespace std;

class SetMqttConfig: public IPlugMessageData {
    public:
        string brokerAddress;

        static unique_ptr<IPlugMessageData> deserialize(const string& serialized);
};

#endif // SET_MQTT_CONFIG_H