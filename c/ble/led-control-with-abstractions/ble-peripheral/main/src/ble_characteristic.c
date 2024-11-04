#include "ble_characteristic.h"

// Function to create a characteristic from a Characteristic struct
struct ble_gatt_chr_def create_ble_characteristic(Characteristic characteristic) {
    // Populate the flags
    ble_gatt_chr_flags flags = 0;
    flags = flags | (characteristic.read ? BLE_GATT_CHR_F_READ : 0);
    ble_gatt_chr_flags acknowledge_writes = characteristic.acknowledge_writes ? BLE_GATT_CHR_F_WRITE : BLE_GATT_CHR_F_WRITE_NO_RSP;
    flags = characteristic.write ? (flags | acknowledge_writes) : flags;

    return (struct ble_gatt_chr_def) 
    {
        .uuid = &characteristic.uuid->u,
        .flags = flags,
        .access_cb = characteristic.on_write,
        .val_handle = characteristic.value_handle
    };
}

// Function to create an array of ble_gatt_chr_def structures from Characteristic array
struct ble_gatt_chr_def* create_ble_characteristics(Characteristic *characteristics, uint8_t characteristics_length){
    // Allocate memory for the ble_gatt_chr_def array + 1 for the terminator
    struct ble_gatt_chr_def* gatt_characteristics = malloc((characteristics_length + 1) * sizeof(struct ble_gatt_chr_def));

    // Create each characteristic
    for (int index = 0; index < characteristics_length; index++) {
        gatt_characteristics[index] = create_ble_characteristic(characteristics[index]);
    }

    // Add the terminator { 0 } at the end
    gatt_characteristics[characteristics_length] = (struct ble_gatt_chr_def){ 0 };

    return gatt_characteristics;
}

// Function to compare two ble_gatt_chr_def structs and print differences
void compare_ble_gatt_chr_def(const struct ble_gatt_chr_def *a, const struct ble_gatt_chr_def *b) {
    if (a->uuid != b->uuid) {
        printf("UUIDs are different: %p vs %p\n", a->uuid, b->uuid);
    } else {
        printf("UUIDs are the same: %p\n", a->uuid);
    }

    if (a->access_cb != b->access_cb) {
        printf("Access callbacks are different: %p vs %p\n", a->access_cb, b->access_cb);
    } else {
        printf("Access callbacks are the same: %p\n", a->access_cb);
    }

    if (a->flags != b->flags) {
        printf("Flags are different: %d vs %d\n", a->flags, b->flags);
    } else {
        printf("Flags are the same: %d\n", a->flags);
    }

    if (a->val_handle != b->val_handle) {
        printf("Value handles are different: %p vs %p\n", a->val_handle, b->val_handle);
    } else {
        printf("Value handles are the same: %p\n", a->val_handle);
    }
}
