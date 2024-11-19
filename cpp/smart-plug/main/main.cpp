#include "WifiService.h"
#include "BleAdvertiser.h"
#include "Timer.h"
#include "AcDimmer.h"
#include "include/smart_plug.h"

extern "C" {
    #include <string.h>
    #include <inttypes.h>
    #include "esp_wifi.h"
    #include "esp_event.h"
    #include "esp_log.h"
}

static const char *TAG = "smart plug";

// BLE constants
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

// Automation IO service
static ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(0x1815);
static ble_uuid128_t led_chr_uuid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

void bleInit() {
    // Create the services
    ESP_LOGI(TAG, "creating BLE Service for Wifi");
    std::vector<BleService> services = {
        // Create the wifi service
        BleService(
            &auto_io_svc_uuid, // TODO: Update uuid
            {
                // Create the SSID characteristic
                BleCharacteristic(
                    &led_chr_uuid, // TODO: Update uuid
                    [](std::vector<std::byte> data) -> int {
                        // TODO: Implement the write callback
                        return 0;
                    },
                    []() -> std::vector<std::byte> {
                        // TODO: Implement the read callback
                        return std::vector<std::byte>();
                    },
                    true, // Characteristic can be read
                    true, // Characteristic can be written
                    false // Characteristic does not acknowledge writes. We do this because the Dotnet BLE library does not support write acknowledgment
                ),
                // Create the password characteristic
                BleCharacteristic(
                    &led_chr_uuid, // TODO: Update uuid
                    [](std::vector<std::byte> data) -> int {
                        // TODO: Implement the write callback
                        return 0;
                    },
                    []() -> std::vector<std::byte> {
                        // TODO: Implement the read callback
                        return std::vector<std::byte>();
                    },
                    false, // Characteristic can not be read
                    true, // Characteristic can be written
                    false // Characteristic does not acknowledge writes. We do this because the Dotnet BLE library does not support write acknowledgment
                ),
                // Create a characteristic for the connection state
                BleCharacteristic(
                    &led_chr_uuid, // TODO: Update uuid
                    [](std::vector<std::byte> data) -> int {
                        // TODO: Implement the write callback
                        return 0;
                    },
                    []() -> std::vector<std::byte> {
                        // TODO: Implement the read callback
                        return std::vector<std::byte>();
                    },
                    false, // Characteristic can not be read
                    true, // Characteristic can be written
                    false // Characteristic does not acknowledge writes. We do this because the Dotnet BLE library does not support write acknowledgment
                )
            }
        ),
        // Create the smart plug service
        BleService(
            &auto_io_svc_uuid, // TODO: update uuid
            {
                // Create the brightness characteristic
                BleCharacteristic(
                    &led_chr_uuid, // TODO: Update uuid
                    [](std::vector<std::byte> data) -> int {
                        // TODO: Implement the write callback
                        return 0;
                    },
                    []() -> std::vector<std::byte> {
                        // TODO: Implement the read callback
                        return std::vector<std::byte>();
                    },
                    true, // Characteristic can be read
                    true, // Characteristic can be written
                    false // Characteristic does not acknowledge writes. We do this because the Dotnet BLE library does not support write acknowledgment
                )
            }
        )
    };

    // Initialize the Ble Advertiser
    ESP_LOGI(TAG, "initializing BLE Advertiser");
    BleAdvertiser::init("Smart Plug", BLE_GAP_APPEARANCE_GENERIC_TAG, BLE_GAP_LE_ROLE_PERIPHERAL, services);

    // Advertise the Ble Device
    ESP_LOGI(TAG, "advertising BLE Device");
    BleAdvertiser::advertise();
}

static uint8_t maxReconnectAttempts = 5;
static uint8_t reconnectAttempts = 0;
void wifiInit() {
    WifiService::onDisconnect = []() {
        ESP_LOGI(TAG, "wifi disconnected");

        // Attempt to reconnect
        if (reconnectAttempts < maxReconnectAttempts) {
            ESP_LOGI(TAG, "attempting to reconnect");
            reconnectAttempts++;
            WifiService::startConnect(
                {
                    "denhac",
                    "denhac rules"
                }
            );
        } else {
            ESP_LOGI(TAG, "max reconnect attempts reached");
        }
    };
    WifiService::onConnect = []() {
        ESP_LOGI(TAG, "Wifi connected");
        reconnectAttempts = 0;
    };
    WifiService::init();
    WifiService::startConnect(
        {
            "denhac",
            "denhac rules"
        }
    );
}

extern "C" void app_main(void)
{
    // Initiate the Wifi Service
    // wifiInit();

    // Initiate the Ble Advertiser
    // bleInit();



    // Initiate the ac dimmer
    // Config for non-dimmable LED
    AcDimmer acDimmer(32, 25, 1000, 5000, 1200);

    uint8_t brightness = 255;
    while(1) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        acDimmer.setBrightness(brightness);
        brightness--;
    }
}
