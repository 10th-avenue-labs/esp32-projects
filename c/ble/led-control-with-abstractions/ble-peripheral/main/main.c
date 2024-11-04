/* Includes */
/* STD APIs */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* ESP APIs */
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

/* FreeRTOS APIs */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

/* NimBLE GAP APIs */
#include "services/gap/ble_svc_gap.h"

/* NimBLE GATT APIs */
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

/* My API's*/
#include "ble_service.h"
#include "ble_advertiser.h"

/* Defines */
#define TAG "NimBLE_Beacon"
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_URI_PREFIX_HTTPS 0x17
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

// Turn off LED
static void led_off() {
    // Turn off led
    ESP_LOGI(TAG, "led turned off in func");
}

// Turn on LED
static void led_on() {
    // Turn on led
    ESP_LOGI(TAG, "led turned on in func");
}

// Automation IO service
static ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(0x1815);
static uint16_t led_chr_val_handle;
static ble_uuid128_t led_chr_uuid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

int on_write(uint8_t* data, uint8_t data_length) {
    // Verify that the data is only 1 byte
    if (data_length != 1) {
        ESP_LOGE(TAG, "unexpected data length received, expected length 1 instead found: %d", data_length);
        return BLE_ATT_ERR_UNLIKELY;
    }

    // Read the data into a bool
    bool state = data[0];
    if (state) {
        led_on();
    } else {
        led_off();
    }

    return 0;
}

// Access to the LED characteristic
static int led_chr_access
(
    uint16_t conn_handle,               // Connection handle
    uint16_t attr_handle,               // The attribute handle - usually a characteristic handle
    struct ble_gatt_access_ctxt *ctxt,  // The context for access to a GATT characteristic or descriptor. This holds the operation and data if the operation is a write
    void *arg
) {
    // Handle access events
    switch (ctxt->op) {
        // Read characteristic
        case BLE_GATT_ACCESS_OP_READ_CHR:
            ESP_LOGE(TAG, "operation not implemented, opcode: %d", ctxt->op);
            return BLE_ATT_ERR_UNLIKELY;
        // Write characteristic
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            // Verify the connection handle
            if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                // The characteristic was written to by a connected device
                ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                        conn_handle, attr_handle);
            } else {
                // The characteristic was written to by the nimble stack
                ESP_LOGI(TAG,
                        "characteristic write by nimble stack; attr_handle=%d",
                        attr_handle);
            }

            // Verify the attribute handle
            // TODO: This is likely where we would break out the delegation to the specific characteristic after performing a lookup
            if (attr_handle != led_chr_val_handle) {
                // An unknown characteristic was written to
                // TODO: This will likely happen when we go to implement more characteristics on a service
                ESP_LOGE(TAG, "unknown attribute handle: %d", attr_handle);
                return BLE_ATT_ERR_UNLIKELY;
            }

            // Call the function delegate
            return on_write(ctxt->om->om_data, ctxt->om->om_len);
        // Read descriptor
        case BLE_GATT_ACCESS_OP_READ_DSC:
            ESP_LOGE(TAG, "operation not implemented, opcode: %d", ctxt->op);
            return BLE_ATT_ERR_UNLIKELY;
        // Write descriptor
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            ESP_LOGE(TAG, "operation not implemented, opcode: %d", ctxt->op);
            return BLE_ATT_ERR_UNLIKELY;
        // Unknown event
        default:
            ESP_LOGE(TAG,
                "unexpected access operation to led characteristic, opcode: %d",
                ctxt->op);
            return BLE_ATT_ERR_UNLIKELY;
    }

    // Control shouldn't reach here
}

void ble_advertiser_advertise_task(void *arg) {
    BleAdvertiser *bleAdvertiser = (BleAdvertiser *)arg;
    ble_advertiser_advertise(bleAdvertiser);
}

void app_main(void) {
    // Create GATT characteristics
    Characteristic characteristics[] = {
        {
            .read = true,
            .write = true,
            .acknowledge_writes = false, // The dotnet library used in conjunction here fails when acknowledgements are sent
            .on_write = led_chr_access,
            .uuid = &led_chr_uuid,
            .value_handle = &led_chr_val_handle
        }
    };

    // Create GATT services
    BleService services[] = {
        {
            .characteristics = characteristics,
            .characteristics_length = 1,
            .uuid = &auto_io_svc_uuid
        }
    };

    // Create the BLE advertiser
    struct BleAdvertiser bleAdvertiser = {
        .device_name = "Montana's_Beacon",
        .services = services,
        .service_count = 1
    };

    // Initialize the BLE advertiser
    bool success = ble_advertiser_init(&bleAdvertiser);
    if(!success) {
        ESP_LOGE(TAG, "Could not initialize BLE advertiser");
    }

    // Start the advertizing task
    xTaskCreate(ble_advertiser_advertise_task, "NimBLE Host", 4*1024, (void *) &bleAdvertiser, 5, NULL);
    while(1) {
        // ESP_LOGI(TAG, "Handle: '%d'", led_chr_val_handle);
        vTaskDelay(1000);
    }
    ESP_LOGI(TAG, "Exiting main thread");
    return;
}