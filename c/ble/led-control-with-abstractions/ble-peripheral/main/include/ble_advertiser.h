#ifndef BLE_ADVERTISER_H
#define BLE_ADVERTISER_H

#include <stdint.h>
#include <stdbool.h>
#include "ble_service.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_log.h"

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

/* Library function declarations */
void ble_store_config_init(void); // For some reason we need to manually declare this one? Not sure why as it lives in a library

typedef struct BleAdvertiser {
    char *device_name;                          // Device name for advertising
    uint16_t device_appearance;                 // BLE appearance (icon or type)
    BleService *services;                       // Array of GATT services
    uint8_t service_count;                      // Number of services
    bool initiated;                             // Initialization status flag
} BleAdvertiser;

bool ble_advertiser_init(BleAdvertiser *bleAdvertiser);

void ble_advertiser_advertise(BleAdvertiser *bleAdvertiser);

#endif // BLE_ADVERTISER_H
