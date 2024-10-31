/* Includes */

/* My API's*/

#include "ble_advertiser.h"
#include "ble_characteristic.h"
#include "ble_service.h"

/* Defines */
#define TAG "NimBLE_Beacon"
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_URI_PREFIX_HTTPS 0x17
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

// Automation IO service
static ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(0x1815);
static uint16_t led_chr_val_handle;
static ble_uuid128_t led_chr_uuid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

extern "C" void app_main() {

    return;
}