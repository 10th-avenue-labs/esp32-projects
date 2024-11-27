#include "BleService.h"

const char* TAG = "BleService";

BleService::BleService(std::string uuid, std::vector<BleCharacteristic> characteristics)
{
    this->uuid = uuid;
    this->characteristics = std::move(characteristics);

    // Populate the UUID structure from the UUID string
    ESP_ERROR_CHECK(BleCharacteristic::uuidStringToUuid(uuid, *uuidPointer));
};


BleService::~BleService()
{
    // FIXME: Implement memory management, currently, this is a memory leak
    // free(uuidPointer);
}


ble_uuid_any_t* BleService::getUuidPointer() {
    return uuidPointer;
}