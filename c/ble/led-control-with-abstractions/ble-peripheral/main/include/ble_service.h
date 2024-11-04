#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include "ble_characteristic.h" // Include the header for Characteristic and ble_gatt_chr_def
#include <stdlib.h>

// Define the structure for Service
typedef struct {
    ble_uuid16_t *uuid;                         // Pointer to service UUID
    Characteristic *characteristics;            // Array of characteristics
    uint8_t characteristics_length;             // Length of characteristics array
} BleService;

// Function declarations
struct ble_gatt_svc_def create_ble_service(BleService service);
struct ble_gatt_svc_def* create_ble_services(BleService *services, uint8_t services_length);

#endif // BLE_SERVICE_H
