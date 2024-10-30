#include "BleHost.h"
#include "Result.h"

using namespace std;

class BleHost {

    private:
        string deviceName;

    public:
        BleHost(string deviceName) : deviceName(deviceName) {
            // Do stuff
        }

        Result<esp_err_t> initialize() {
            esp_err_t ret;

            // Initialise the non-volatile flash storage (NVS)
            ret = nvs_flash_init();
            if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
                ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

                // Erase and re-try if necessary
                ESP_ERROR_CHECK(nvs_flash_erase());
                ret = nvs_flash_init();
            }
            // Verify the response
            if (ret != ESP_OK) {
                return Result<esp_err_t>::failure(format("failed to initialize nvs flash, error code: %d ", ret));
                ESP_LOGE(deviceName.c_str(), );
                return;
            }

            
            // Initialise the controller and nimble host stack 


        }
};