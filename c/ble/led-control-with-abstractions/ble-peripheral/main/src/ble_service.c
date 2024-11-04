#include "ble_service.h"

// Function to create a single service
struct ble_gatt_svc_def create_ble_service(BleService service) {
    return (struct ble_gatt_svc_def) {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &service.uuid->u,
        .characteristics = create_ble_characteristics(service.characteristics, service.characteristics_length)
    };
}

// Function to create an array of services
struct ble_gatt_svc_def* create_ble_services(BleService *services, uint8_t services_length) {
    // Allocate memory for the ble_gatt_svc_def array + 1 for the terminator
    struct ble_gatt_svc_def* gatt_services = malloc((services_length + 1) * sizeof(struct ble_gatt_svc_def));

    // Create each service
    for (int index = 0; index < services_length; index++) {
        gatt_services[index] = create_ble_service(services[index]);
    }

    // Add the terminator { 0 } at the end
    gatt_services[services_length] = (struct ble_gatt_svc_def){ 0 };

    return gatt_services;
}
