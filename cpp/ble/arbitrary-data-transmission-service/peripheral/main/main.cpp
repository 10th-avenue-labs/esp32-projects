#include <algorithm>
#include <string>
#include "BleAdvertiser.h"
#include "BleService.h"
#include "AdtService.h"

extern "C"
{
    #include <string.h>
    #include <inttypes.h>
    #include "esp_log.h"
    #include <host/ble_uuid.h>
    #include <esp_err.h>
}

static const char *TAG = "adt service";

// BLE constants
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

extern "C" void app_main(void)
{
    // Create a new ADT service
    AdtService adtService("00000000-0000-0000-0000-000000012346", "12345670-0000-0000-0000-000000000000", "12345679-0000-0000-0000-000000000000",
                          [](std::vector<std::byte> data)
                          {
                              ESP_LOGI(TAG, "received message of size %d", data.size());

                              // Convert the data to a string
                              std::string str(reinterpret_cast<const char *>(data.data()), data.size());

                              // Print the data
                              ESP_LOGI(TAG, "received data: %s", str.c_str());
                          });

    // Call init with inline map initialization
    BleAdvertiser::init("ADT Test", BLE_GAP_APPEARANCE_GENERIC_TAG, BLE_GAP_LE_ROLE_PERIPHERAL, {*adtService.getBleService()});

    // Advertise the BLE device
    BleAdvertiser::advertise();
}
