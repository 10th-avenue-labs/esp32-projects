#ifndef PLUG_CONFIG_H
#define PLUG_CONFIG_H

#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <tuple>
#include <memory>
#include <nvs_handle.hpp>
#include "cJSON.h"
#include "AcDimmerConfig.h"
#include "WifiConfig.h"
#include "BleConfig.h"
#include "MqttConfig.h"

using namespace std;

class PlugConfig {
    public:
        shared_ptr<BleConfig> bleConfig;
        shared_ptr<AcDimmerConfig> acDimmerConfig;
        shared_ptr<WifiConfig> wifiConfig;
        shared_ptr<MqttConfig> mqttConfig;

        static tuple<esp_err_t, unique_ptr<PlugConfig>> readPlugConfig(const string& namespaceValue, const string& key);

        unique_ptr<cJSON, void (*)(cJSON *item)> serialize();
        static PlugConfig deserialize(const string& serialized);
};

#endif // PLUG_CONFIG_H