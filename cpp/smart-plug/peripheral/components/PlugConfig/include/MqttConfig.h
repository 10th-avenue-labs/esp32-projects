#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

#include <memory>
#include "cJSON.h"

using namespace std;

class MqttConfig
{
public:
    string brokerAddress;
    string deviceId;
    string jwt;

    unique_ptr<cJSON, void (*)(cJSON *item)> serialize();
    static MqttConfig deserialize(const string &serialized);
};

#endif // MQTT_CONFIG_H