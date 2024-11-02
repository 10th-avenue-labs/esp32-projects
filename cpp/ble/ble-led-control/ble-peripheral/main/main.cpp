/* Includes */

/* My API's*/

#include "ble_advertiser.h"
#include "ble_characteristic.h"
#include "ble_service.h"

extern "C"
{
    #include <esp_log.h>
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
}

/* Defines */
#define TAG "MAIN"
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

// Automation IO service
static ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(0x1815);
static ble_uuid128_t led_chr_uuid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

static uint8_t characteristicValue = 0;
int onWrite(std::vector<std::byte> data) {
    characteristicValue = std::to_integer<uint8_t>(data[0]);
    return 0;
}

std::vector<std::byte> onRead(void) {
    return {std::byte(characteristicValue)};
}

void bleAdvertiserTask(void *arg) {
    BleAdvertiser::advertise();
}

extern "C" void app_main() {
    // Create a characteristic for the LED
    BleCharacteristic led = BleCharacteristic(
        &led_chr_uuid,
        onWrite,
        onRead,
        true,
        true,
        false
    );

    // Create a service for the LED
    BleService service = BleService(
        &auto_io_svc_uuid,
        {led}
    );

    // Initialize the advertiser
    bool success = BleAdvertiser::init(
        "Montana's_Beacon",
        BLE_GAP_APPEARANCE_GENERIC_TAG,
        BLE_GAP_LE_ROLE_PERIPHERAL,
        {service}
    );

    // Check if the advertiser was initialized successfully
    if (success) {
        ESP_LOGI(TAG, "BLE advertiser initialized successfully");
    } else {
        ESP_LOGE(TAG, "BLE advertiser failed to initialize");
    }

    // Start the advertizing task
    xTaskCreate(bleAdvertiserTask, "NimBLE Host", 4*1024, nullptr, 5, NULL);
    while(1) {
        // ESP_LOGI(TAG, "Handle: '%d'", led_chr_val_handle);
        vTaskDelay(1000);
    }

    ESP_LOGI(TAG, "Exiting main thread");
    return;
}