#include "ble_advertiser.h"



extern "C"
{
    #include <esp_log.h>
    #include <esp_err.h>
    #include <nvs_flash.h>
    #include <nimble/nimble_port.h>
    #include <services/gap/ble_svc_gap.h>
    #include <host/ble_gatt.h>
    #include <services/gatt/ble_svc_gatt.h>
    #include <host/ble_hs.h>
}

static const std::string TAG = "BLE_ADVERTISER";


BleAdvertiser::BleAdvertiser
(
    std::string deviceName,
    uint16_t deviceAppearance,
    std::vector<BleService> services
):
    deviceName(deviceName),
    deviceAppearance(deviceAppearance),
    services(services),
    initiated(false)
{
    // No impl
}

bool BleAdvertiser::init() {
    // Initialise the non-volatile flash storage (NVS)
    ESP_LOGI(deviceName.c_str(), "initializing nvs flash");
    esp_err_t response = nvs_flash_init();

    // Attempt to recover if an error occurred
    if (response != ESP_OK) {
        // Check if a recoverable error occured
        if (response == ESP_ERR_NVS_NO_FREE_PAGES ||
            response == ESP_ERR_NVS_NEW_VERSION_FOUND) {

            // Erase and re-try if necessary
            // Note: This will erase the nvs flash.
            // TODO: We should consider alternative impls here. Erasing the NVS could be a very unwanted side-effect
            ESP_LOGI(deviceName.c_str(), "erasing nvs flash");
            ESP_ERROR_CHECK(nvs_flash_erase());
            response = nvs_flash_init();

            if (response != ESP_OK) {
                ESP_LOGE(deviceName.c_str(), "failed to initialize nvs flash, error code: %d ", response);
                return false;
            };
        }
    }

    // Initialise the controller and nimble host stack 
    response = nimble_port_init();
    if (response != ESP_OK) {
        ESP_LOGE(deviceName.c_str(), "failed to initialize nimble stack, error code: %d ",
            response);
        return false;
    }

    // Initialize the Generic Attribute Profile (GAP)
    response = gapInit();
    if (response != 0) {
        ESP_LOGE(deviceName.c_str(), "failed to initialize GAP service, error code: %d", response);
        return false;
    }

    // Create the GATT services
    struct ble_gatt_svc_def *serviceDefinitions = createServiceDefinitions(services);

    // Initialize the Generic ATTribute Profile (GATT) server <- I know, it's a bad acronym
    response = gattSvcInit(serviceDefinitions);
    if (response != 0) {
        ESP_LOGE(deviceName.c_str(), "failed to initialize GATT server, error code: %d", response);
        return false;
    }

    // Initialize the nimble host configuration
    nimble_host_config_init();

    return true;
}





static int characteristicAccessHandler
(
    uint16_t conn_handle,
    uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt,
    void *arg
) {
    // Get the injected BleAdvertiser
    BleAdvertiser *advertiser = (BleAdvertiser*)arg;

    // Handle access events
    switch (ctxt->op) {
        // Read characteristic
        case BLE_GATT_ACCESS_OP_READ_CHR:
            ESP_LOGE(advertiser->deviceName.c_str(), "operation not implemented, opcode: %d", ctxt->op);
            return BLE_ATT_ERR_UNLIKELY;
        // Write characteristic
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            // Verify the connection handle
            if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                // The characteristic was written to by a connected device
                ESP_LOGI(advertiser->deviceName.c_str(), "characteristic write; conn_handle=%d attr_handle=%d",
                        conn_handle, attr_handle);
            } else {
                // The characteristic was written to by the nimble stack
                ESP_LOGI(advertiser->deviceName.c_str(),
                        "characteristic write by nimble stack; attr_handle=%d",
                        attr_handle);
            }

            // // Verify the attribute handle
            // // TODO: This is likely where we would break out the delegation to the specific characteristic after performing a lookup
            // if (attr_handle != led_chr_val_handle) {
            //     // An unknown characteristic was written to
            //     // TODO: This will likely happen when we go to implement more characteristics on a service
            //     ESP_LOGE(deviceName.c_str(), "unknown attribute handle: %d", attr_handle);
            //     return BLE_ATT_ERR_UNLIKELY;
            // }

            // Call the function delegate
            // return on_write(ctxt->om->om_data, ctxt->om->om_len);

            // TODO: This is where we would call the onWrite function for the characteristic
            ESP_LOGI(advertiser->deviceName.c_str(), "characteristic written to");
        // Read descriptor
        case BLE_GATT_ACCESS_OP_READ_DSC:
            ESP_LOGE(advertiser->deviceName.c_str(), "operation not implemented, opcode: %d", ctxt->op);
            return BLE_ATT_ERR_UNLIKELY;
        // Write descriptor
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            ESP_LOGE(advertiser->deviceName.c_str(), "operation not implemented, opcode: %d", ctxt->op);
            return BLE_ATT_ERR_UNLIKELY;
        // Unknown event
        default:
            ESP_LOGE(advertiser->deviceName.c_str(),
                "unexpected access operation to led characteristic, opcode: %d",
                ctxt->op);
            return BLE_ATT_ERR_UNLIKELY;
    }

    // Control shouldn't reach here
}











int BleAdvertiser::gapInit() {
    // Local variables to be reused
    int response = 0;

    // Initialize the gap service
    ble_svc_gap_init();

    // Set the gap device name
    response = ble_svc_gap_device_name_set(deviceName.c_str());
    if (response != 0) {
        ESP_LOGE(deviceName.c_str() , "failed to set device name to %s, error code: %d",
                 deviceName.c_str(), response);
        return response;
    }

    // Set the gap appearance
    response = ble_svc_gap_device_appearance_set(deviceAppearance);
    if (response != 0) {
        ESP_LOGE(deviceName.c_str(), "failed to set device appearance, error code: %d", response);
        return response;
    }
    return response;
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int BleAdvertiser::gattSvcInit(struct ble_gatt_svc_def *serviceDefinitions) {
    // Local variables to reuse
    int response;

    // 1. GATT service initialization
    ble_svc_gatt_init();

    // 2. Update GATT services counter
    response = ble_gatts_count_cfg(serviceDefinitions);
    if (response != 0) {
        return response;
    }

    // 3. Add GATT services
    response = ble_gatts_add_svcs(serviceDefinitions);
    if (response != 0) {
        return response;
    }

    return 0;
}

inline static void format_addr(char *addr_str, uint8_t addr[]) {
    sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1],
            addr[2], addr[3], addr[4], addr[5]);
}

// Initialize and start advertizing
void adv_init(void) {
    // Local variables to be reused
    int rc = 0;
    char addr_str[18] = {0};

    // Make sure we have a proper BT address
    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG.c_str(), "device does not have any available bt address!");
        return;
    }

    // Determine the BL address type to use while advertizing
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG.c_str(), "failed to infer address type, error code: %d", rc);
        return;
    }

    // Copy the device address to addr_val
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG.c_str(), "failed to copy device address, error code: %d", rc);
        return;
    }
    format_addr(addr_str, addr_val);
    ESP_LOGI(TAG.c_str(), "device address: %s", addr_str);

    // Start advertising.
    start_advertising();
}

/* Private functions */
/*
 *  Stack event callback functions
 *      - on_stack_reset is called when host resets BLE stack due to errors
 *      - on_stack_sync is called when host has synced with controller
 */
void onStackReset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG.c_str(), "nimble stack reset, reset reason: %d", reason);
}

void onStackSync(void) {
    // This function will be called once the nimble host stack is synced with the BLE controller
    // Once the stack is synced, we can do advertizing initialization and begin advertising

    // Initialize and start advertizing
    adv_init();
}

/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    // Local variables to be reused
    char buf[BLE_UUID_STR_LEN];

    // Handle GATT attributes register events
    switch (ctxt->op) {

        // Service register event
        case BLE_GATT_REGISTER_OP_SVC:
            ESP_LOGD(TAG.c_str(), "registered service %s with handle=%d",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
            break;

        // Characteristic register event
        case BLE_GATT_REGISTER_OP_CHR:
            ESP_LOGD(TAG.c_str(),
                    "registering characteristic %s with "
                    "def_handle=%d val_handle=%d",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle, ctxt->chr.val_handle);
            break;

        // Descriptor register event
        case BLE_GATT_REGISTER_OP_DSC:
            ESP_LOGD(TAG.c_str(), "registering descriptor %s with handle=%d",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
            break;

        // Unknown event
        default:
            assert(0);
            break;
    }
}


// Initialize the nimble host config
void BleAdvertiser::nimbleHostConfigInit(void) {
    // Set the host callbacks
    // Idk where the ble_hs_cfg variable is are coming from?
    ble_hs_cfg.reset_cb = onStackReset; // TODO: Find out a way to pass the device name to this function for logging and greater control
    ble_hs_cfg.sync_cb = onStackSync; // TODO: Find out a way to pass the device name to this function for logging and greater control
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb; // TODO: Find out a way to pass the device name to this function for logging and greater control

    /* Store host configuration */
    ble_store_config_init();
}

struct ble_gatt_svc_def* BleAdvertiser::createServiceDefinitions(const std::vector<BleService>& services) {
    // Get the number of services
    size_t servicesLength = services.size();

    // Allocate memory for the ble_gatt_svc_def array + 1 for the terminator
    struct ble_gatt_svc_def* gattServices = (struct ble_gatt_svc_def*)malloc((servicesLength + 1) * sizeof(struct ble_gatt_svc_def));

    // Check if malloc was successful
    if (gattServices == nullptr) {
        return nullptr;
    }

    // Create each service
    for (size_t index = 0; index < servicesLength; index++) {
        gattServices[index] = createServiceDefinition(services[index]);
    }

    // Add the terminator { 0 } at the end
    gattServices[servicesLength] = (struct ble_gatt_svc_def){ 0 };

    return gattServices;
}

struct ble_gatt_svc_def BleAdvertiser::createServiceDefinition(BleService service) {
    return (struct ble_gatt_svc_def) {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &service.uuid->u,
        .characteristics = createCharacteristicDefinitions(service.characteristics)
    };
}

// Function to create an array of ble_gatt_chr_def structures from Characteristic array
struct ble_gatt_chr_def* BleAdvertiser::createCharacteristicDefinitions(std::vector<BleCharacteristic> characteristics){
    // Get the number of characteristics
    size_t characteristicsLength = characteristics.size();

    // Allocate memory for the ble_gatt_chr_def array + 1 for the terminator
    struct ble_gatt_chr_def* gattCharacteristics = (ble_gatt_chr_def*)malloc((characteristicsLength + 1) * sizeof(struct ble_gatt_chr_def));

    // Check if malloc was successful
    if (gattCharacteristics == nullptr) {
        return nullptr;
    }

    // Create each characteristic
    for (int index = 0; index < characteristicsLength; index++) {
        gattCharacteristics[index] = createCharacteristicDefinition(characteristics[index]);
    }

    // Add the terminator { 0 } at the end
    gattCharacteristics[characteristicsLength] = (struct ble_gatt_chr_def){ 0 };

    return gattCharacteristics;
}

struct ble_gatt_chr_def BleAdvertiser::createCharacteristicDefinition(BleCharacteristic characteristic) {
    // First create the characteristic handle and link it to it's callback
    uint16_t characteristicHandle = 0;
    characteristicHandlesToCallbacks.insert(std::pair<uint16_t, std::function<int(std::vector<std::byte>)>>(characteristicHandle, characteristic.onWrite));

    // Populate the flags
    ble_gatt_chr_flags flags = 0;
    flags = flags | (characteristic.read ? BLE_GATT_CHR_F_READ : 0);
    ble_gatt_chr_flags acknowledge_writes = characteristic.acknowledgeWrites ? BLE_GATT_CHR_F_WRITE : BLE_GATT_CHR_F_WRITE_NO_RSP;
    flags = characteristic.write ? (flags | acknowledge_writes) : flags;

    return (struct ble_gatt_chr_def)
    {
        .uuid = &characteristic.uuid->u,
        .access_cb = characteristicAccessHandler,
        .arg = this,
        .flags = flags,
        .val_handle = &characteristicHandle,
    };
}