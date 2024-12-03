#include "WifiService.h"
#include "BleAdvertiser.h"
#include "Timer.h"
#include "AcDimmer.h"
#include "MqttClient.h"
#include "AdtService.h"
#include "include/smart_plug.h"

extern "C" {
    #include <string.h>
    #include <inttypes.h>
    #include "esp_wifi.h"
    #include "esp_event.h"
    #include "esp_log.h"
}

static const char *TAG = "SMART_PLUG";

extern "C" void app_main(void)
{
    // TODO: Implement NVS
    // Attempt to read the smart plug brightness from NVS
    // Attempt to read the wifi configuration from NVS
    // Attempt to read the mqtt configuration from NVS
    // Attempt to read the schedule from NVS

    // Read the current plug configuration if it exists
    
}
