#ifndef BLE_CHARACTERISTIC_H
#define BLE_CHARACTERISTIC_H

/* NimBLE GATT APIs */
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

#include <stdio.h>
#include <stdbool.h>

typedef int (*characteristic_access_callback)(
    uint16_t conn_handle,
    uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt,
    void *arg
);

typedef struct {
    ble_uuid128_t *uuid;                        // Pointer to UUID
    characteristic_access_callback on_write;    // Callback for write access
    bool read;                                  // Flag for whether or not to allow read access
    bool write;                                 // Flag for whether or not to allow write access
    bool acknowledge_writes;                    // Flag for write acknowledgment
    void *value_handle;                         // Handle for the characteristic value
} Characteristic;

// Function declarations
struct ble_gatt_chr_def create_ble_characteristic(Characteristic characteristic);
struct ble_gatt_chr_def* create_ble_characteristics(Characteristic *characteristics, uint8_t characteristics_length);

#endif // BLE_CHARACTERISTIC_H
