#include "BleService.h"

static const char* TAG = "BLE_SERVICE";

////////////////////////////////////////////////////////////////////////////////
/// Constructors / Destructors
////////////////////////////////////////////////////////////////////////////////

BleService::BleService(string uuid, vector<BleCharacteristic>&& characteristics)
{
    // Move the characteristics into a shared pointer
    for (auto& characteristic : characteristics) {
        // Convert each Characteristic to a shared_ptr and move it
        this->characteristics.emplace_back(make_shared<BleCharacteristic>(move(characteristic)));
    }

    // TODO: We could (and should) reduce duplication between this and the other constructor

    // Populate the UUID structure from the UUID string
    // TODO: We could make this function return a value most likely
    BleCharacteristic::uuidStringToUuid(uuid, this->uuidDefinition);

    // Create an array of characteristic definitions
    ESP_ERROR_CHECK(createGattCharacteristicDefinitions());
};

BleService::BleService(string uuid, vector<shared_ptr<BleCharacteristic>> characteristics):
    characteristics(characteristics)
{
    // Populate the UUID structure from the UUID string
    // TODO: We could make this function return a value most likely
    BleCharacteristic::uuidStringToUuid(uuid, this->uuidDefinition);

    // Create an array of characteristic definitions
    ESP_ERROR_CHECK(createGattCharacteristicDefinitions());
};

BleService::BleService(BleService&& other)
{
    // Move the UUID definition
    this->uuidDefinition = move(other.uuidDefinition);

    // Move the characteristics
    this->characteristics = move(other.characteristics);

    // Move the characteristic definitions
    this->gattCharacteristicDefinitions = other.gattCharacteristicDefinitions;
    other.gattCharacteristicDefinitions = nullptr;
}

BleService::~BleService()
{
    free(gattCharacteristicDefinitions);
}

////////////////////////////////////////////////////////////////////////////////
/// Friend functions
////////////////////////////////////////////////////////////////////////////////

esp_err_t BleService::populateGattServiceDefinition(ble_gatt_svc_def* gattServiceDefinition) {
    // Create the GATT service definition
    *gattServiceDefinition = (struct ble_gatt_svc_def) {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &this->uuidDefinition.u,
        .characteristics = gattCharacteristicDefinitions
    };

    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////
/// Private Methods
////////////////////////////////////////////////////////////////////////////////

esp_err_t BleService::createGattCharacteristicDefinitions() {
    // Get the number of characteristics
    size_t characteristicsLength = characteristics.size();

    // Check if there are any characteristics
    if (characteristicsLength == 0) {
        ESP_LOGW(TAG, "service has no characteristics. This service may not be discoverable by all consumers, eg web bluetooth");
    }

    // Allocate memory for the ble_gatt_chr_def array + 1 for the terminator
    // Note: This memory must be explicitly de-allocated which is done by the destructor of this class
    gattCharacteristicDefinitions = (ble_gatt_chr_def*)malloc((characteristicsLength + 1) * sizeof(struct ble_gatt_chr_def));

    // Check if malloc was successful
    if (gattCharacteristicDefinitions == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    // Populate each characteristic
    for (int index = 0; index < characteristicsLength; index++) {
        // Populate the characteristic definition
        esp_err_t response = characteristics[index].get()->populateGattCharacteristicDefinition(&gattCharacteristicDefinitions[index]);

        // Check if there was an error
        if (response != ESP_OK) {
            return response;
        }
    }

    // Add the terminator { 0 } at the end
    gattCharacteristicDefinitions[characteristicsLength] = (struct ble_gatt_chr_def){ 0 };

    return ESP_OK;
}
