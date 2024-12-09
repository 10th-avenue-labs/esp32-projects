#ifndef PLUG_CONFIG_H
#define PLUG_CONFIG_H

#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <tuple>
#include <memory>
#include <nvs_handle.hpp>
#include "cJSON.h"
#include "Result.h"
#include "AcDimmerConfig.h"
#include "WifiConfig.h"
#include "BleConfig.h"
#include "MqttConfig.h"

using namespace std;

class PlugConfig {
    public:
        shared_ptr<BleConfig> bleConfig;            // Config for BLE
        shared_ptr<AcDimmerConfig> acDimmerConfig;  // Config for the ac dimmer
        shared_ptr<WifiConfig> wifiConfig;          // Config for wifi
        shared_ptr<MqttConfig> mqttConfig;          // Config for mqtt

        /**
         * @brief Read a plug configuration from NVS returning a deserialized PlugConfig object
         * 
         * @param namespaceValue The namespace to read from
         * @param key The key in the namespace to read from
         * @return Result<shared_ptr<PlugConfig>> A result containing the deserialized PlugConfig object
         */
        static Result<shared_ptr<PlugConfig>> readPlugConfig(const string& namespaceValue, const string& key);

        /**
         * @brief Write a plug config to NVS
         * 
         * @param namespaceValue The namespace to write to
         * @param key The key in the namespace to write to
         * @param plugConfig The plug config to write
         * @return Result<> A result indicating success or failure
         */
        Result<> writePlugConfig(const string& namespaceValue, const string& key, const PlugConfig& plugConfig);

        /**
         * @brief Serialize the plug config to a cJSON object
         * 
         * @return unique_ptr<cJSON, void (*)(cJSON *item)> A unique pointer to the cJSON object
         */
        unique_ptr<cJSON, void (*)(cJSON *item)> serialize();

        /**
         * @brief Deserialize a plug config from a serialized string
         * 
         * @param serialized The serialized string
         * @return PlugConfig The deserialized plug config
         */
        static PlugConfig deserialize(const string& serialized);
};

#endif // PLUG_CONFIG_H