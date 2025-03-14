#pragma once

#include "SmartDevice.h"
#include "SmartPlugConfig.h"
#include "AcDimmer.h"

#include <esp_log.h>

static const char *SMART_PLUG_TAG = "SMART_PLUG";

class SmartPlug : public SmartDevice::SmartDevice
{
public:
    /**
     * @brief Construct a new Smart Plug object
     *
     * @param config The configuration to use for the smart plug
     * @param configUpdatedDelegate The delegate to call when the configuration is updated
     */
    SmartPlug(
        std::unique_ptr<SmartPlugConfig> config,
        std::function<void(void)> configUpdatedDelegate = nullptr) : SmartDevice::SmartDevice(std::move(config),
                                                                                              "SmartPlug",
                                                                                              configUpdatedDelegate),
                                                                     acDimmer(nullptr) {
                                                                         // Register message handlers
                                                                         // TODO
                                                                     };

    /**
     * @brief Initialize the smart plug
     *
     */
    void childInitialize() override
    {
        // Initiate the ac dimmer
        ESP_LOGI(SMART_PLUG_TAG, "initializing AC dimmer");
        acDimmer = make_unique<AcDimmer>(
            config.acDimmerConfig->zcPin,
            config.acDimmerConfig->psmPin,
            config.acDimmerConfig->debounceUs,
            config.acDimmerConfig->offsetLeading,
            config.acDimmerConfig->offsetFalling,
            config.acDimmerConfig->brightness);
    };

private:
    SmartPlugConfig &config = static_cast<SmartPlugConfig &>(*SmartDevice::config);
    unique_ptr<AcDimmer> acDimmer;
};