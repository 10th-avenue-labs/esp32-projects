#include "BleAdvertiser.h"

#define BLE_GAP_URI_PREFIX_HTTPS 0x17
static uint8_t esp_uri[] = {BLE_GAP_URI_PREFIX_HTTPS, '/', '/', 'e', 's', 'p', 'r', 'e', 's', 's', 'i', 'f', '.', 'c', 'o', 'm'};
static char* TAG = "BLE_ADVERTISER";

// Define static members to satisfy the compiler. These will be overwritten by the init function
string BleAdvertiser::deviceName = "esp32_bluetooth";
uint16_t BleAdvertiser::deviceAppearance = 0;
uint8_t BleAdvertiser::deviceRole = 0;
bool BleAdvertiser::initiated = false;
uint8_t BleAdvertiser::deviceAddress[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t BleAdvertiser::deviceAddressType = 0;
uint16_t BleAdvertiser::mtu = 0;
ble_gatt_svc_def* BleAdvertiser::gattServiceDefinitions = nullptr;
vector<shared_ptr<BleService>> BleAdvertiser::services = {};
function<void(shared_ptr<BleDevice> device)> BleAdvertiser::onDeviceConnected = nullptr;
map<uint16_t, shared_ptr<BleDevice>> BleAdvertiser::connectedDevicesByHandle = {};
map<uint16_t*, shared_ptr<BleCharacteristic>> BleAdvertiser::characteristicsByHandle = {};

////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////

bool BleAdvertiser::init(
    string deviceName,
    uint16_t deviceAppearance,
    uint8_t deviceRole,
    vector<shared_ptr<BleService>>&& services,
    function<void(shared_ptr<BleDevice> device)> onDeviceConnected
) {
    // Initialize static variables
    BleAdvertiser::deviceName = deviceName;
    BleAdvertiser::deviceAppearance = deviceAppearance;
    BleAdvertiser::deviceRole = deviceRole;
    BleAdvertiser::services = move(services);
    BleAdvertiser::onDeviceConnected = onDeviceConnected;

    // Initialise the non-volatile flash storage (NVS)
    ESP_LOGI(TAG, "initializing nvs flash");
    esp_err_t response = nvs_flash_init();

    // Attempt to recover if an error occurred
    if (response != ESP_OK) {
        // Check if a recoverable error occured
        if (response == ESP_ERR_NVS_NO_FREE_PAGES ||
            response == ESP_ERR_NVS_NEW_VERSION_FOUND) {

            // Erase and re-try if necessary
            // Note: This will erase the nvs flash.
            // TODO: We should consider alternative impls here. Erasing the NVS could be a very unwanted side-effect
            ESP_LOGI(TAG, "erasing nvs flash");
            ESP_ERROR_CHECK(nvs_flash_erase());
            response = nvs_flash_init();

            if (response != ESP_OK) {
                ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", response);
                return false;
            };
        }
    }

    // Initialise the controller and nimble host stack 
    response = nimble_port_init();
    if (response != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ",
            response);
        return false;
    }

    // Initialize the Generic Attribute Profile (GAP)
    response = gapInit();
    if (response != 0) {
        ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", response);
        return false;
    }

    // Create the GATT services
    ESP_ERROR_CHECK(createGattServiceDefinitions());

    // Initialize the Generic ATTribute Profile (GATT) server <- I know, it's a bad acronym
    response = gattSvcInit(gattServiceDefinitions);
    if (response != 0) {
        ESP_LOGE(TAG, "failed to initialize GATT server, error code: %d", response);
        return false;
    }

    // Initialize the nimble host configuration
    nimbleHostConfigInit();

    // Set the initialized flag to true
    initiated = true;

    return true;
}

void BleAdvertiser::advertise(void) {
    // Log the start of the task
    ESP_LOGI(TAG, "nimble host task has been started by %s", deviceName.c_str());

    // This function won't return until nimble_port_stop() is executed
    nimble_port_run();

    // Clean up at exit
    vTaskDelete(NULL);
}

void BleAdvertiser::shutdown(void) {
    // Log the shutdown
    ESP_LOGI(TAG, "shutting down");

    // Stop the nimble stack
    nimble_port_stop();

    // Set the initiated flag to false
    initiated = false;

    // Free the memory allocated for the GATT service definitions
    free(gattServiceDefinitions);
}

uint16_t BleAdvertiser::getMtu(void) {
    return BleAdvertiser::mtu;
}

////////////////////////////////////////////////////////////////////////////
// BLE helper functions
////////////////////////////////////////////////////////////////////////////

inline void BleAdvertiser::formatAddress(char *addressString, uint8_t address[]) {
    sprintf(addressString, "%02X:%02X:%02X:%02X:%02X:%02X", address[0], address[1],
            address[2], address[3], address[4], address[5]);
}

void BleAdvertiser::printConnectionDescription(struct ble_gap_conn_desc *connectionDescription) {
    // Local variables to be reused
    char addressString[18] = {0};

    // Connection handle
    ESP_LOGI(TAG, "connection handle: %d", connectionDescription->conn_handle);

    // Local ID address
    formatAddress(addressString, connectionDescription->our_id_addr.val);
    ESP_LOGI(TAG, "device id address: type=%d, value=%s",
             connectionDescription->our_id_addr.type, addressString);

    // Peer ID address
    formatAddress(addressString, connectionDescription->peer_id_addr.val);
    ESP_LOGI(TAG, "peer id address: type=%d, value=%s", connectionDescription->peer_id_addr.type,
             addressString);

    // Connection info
    ESP_LOGI(TAG,
             "conn_itvl=%d, conn_latency=%d, supervision_timeout=%d, "
             "encrypted=%d, authenticated=%d, bonded=%d\n",
             connectionDescription->conn_itvl, connectionDescription->conn_latency, connectionDescription->supervision_timeout,
             connectionDescription->sec_state.encrypted, connectionDescription->sec_state.authenticated,
             connectionDescription->sec_state.bonded);
}

int BleAdvertiser::gapInit(void) {
    // Local variables to be reused
    int response = 0;

    // Initialize the gap service
    ble_svc_gap_init();

    // Set the gap device name
    response = ble_svc_gap_device_name_set(deviceName.c_str());
    if (response != 0) {
        ESP_LOGE(TAG , "failed to set device name to %s, error code: %d",
                 deviceName.c_str(), response);
        return response;
    }

    // Set the gap appearance
    response = ble_svc_gap_device_appearance_set(deviceAppearance);
    if (response != 0) {
        ESP_LOGE(TAG, "failed to set device appearance, error code: %d", response);
        return response;
    }
    return response;
}

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

void BleAdvertiser::nimbleHostConfigInit(void) {
    // Set the host callbacks
    // Idk where the ble_hs_cfg variable is are coming from?
    ble_hs_cfg.reset_cb = onStackReset;
    ble_hs_cfg.sync_cb = onStackSync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.gatts_register_cb = gattSvrRegisterCb;

    // Store host configuration
    ble_store_config_init();
}

int BleAdvertiser::gapEventHandler(struct ble_gap_event *event, void *arg) {
    // Local variables to be reused
    int response = 0;
    struct ble_gap_conn_desc connectionDescription;

    // Handle different GAP events
    switch (event->type) {
        // Connect event
        case BLE_GAP_EVENT_CONNECT: {
            // A new connection was established or a connection attempt failed.
            ESP_LOGI(TAG, "connection %s; status=%d",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);

            // Check if the connection failed
            if(event->connect.status != 0) {
                startAdvertising();
                return response;
            }

            // Check connection handle
            response = ble_gap_conn_find(event->connect.conn_handle, &connectionDescription);
            if (response != 0) {
                ESP_LOGE(TAG,
                        "failed to find connection by handle, error code: %d",
                        response);
                return response;
            }

            // Print connection descriptor
            printConnectionDescription(&connectionDescription);

            // Try to update connection parameters
            struct ble_gap_upd_params params = {.itvl_min = connectionDescription.conn_itvl,
                                                .itvl_max = connectionDescription.conn_itvl,
                                                .latency = 3,
                                                .supervision_timeout =
                                                    connectionDescription.supervision_timeout};
            response = ble_gap_update_params(event->connect.conn_handle, &params);
            if (response != 0) {
                ESP_LOGE(
                    TAG,
                    "failed to update connection parameters, error code: %d",
                    response);
                return response;
            }

            // Exchange MTU
            ble_gattc_exchange_mtu(event->connect.conn_handle, mtuEventHandler, NULL);

            // Create a new BleDevice
            shared_ptr<BleDevice> device = make_shared<BleDevice>(
                event->connect.conn_handle
            );

            // Add the device to the list of connected devices
            connectedDevicesByHandle[event->connect.conn_handle] = device;

            // Call the onDeviceConnected callback
            if (onDeviceConnected != nullptr) {
                onDeviceConnected(device);
            }

            return response;
        }
        // Disconnect event
        case BLE_GAP_EVENT_DISCONNECT:
            // A connection was terminated, print connection descriptor
            ESP_LOGI(TAG, "disconnected from peer; reason=%d",
                    event->disconnect.reason);

            // Remove the device from the list of connected devices
            connectedDevicesByHandle.erase(event->disconnect.conn.conn_handle);

            // TODO: Could add an onDisconnect callback here

            // Restart advertising
            startAdvertising();
            return response;

        case BLE_GAP_EVENT_SUBSCRIBE: {
            // A client has subscribed to notifications or indications
            ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                    event->subscribe.conn_handle, event->subscribe.attr_handle);

            // Get the device that subscribed
            shared_ptr<BleDevice> device = connectedDevicesByHandle[event->subscribe.conn_handle];

            // Find the characteristic subscribed to by it's handle
            shared_ptr<BleCharacteristic> characteristic = nullptr;
            for(auto const& [key, val] : characteristicsByHandle) {
                if (*key == event->subscribe.attr_handle) {
                    characteristic = val;
                    break;
                }
            }

            if (characteristic == nullptr) {
                // There are some default characteristics added by the nimble stack that do not exist in this map, but that can be subscribed to
                return response;
            }

            // Call the onSubscribe callback
            if (characteristic.get()->onSubscribe != nullptr) {
                characteristic.get()->onSubscribe(device);
            }

            return response;
        }
        // Connection parameters update event
        case BLE_GAP_EVENT_CONN_UPDATE:
            // The central has updated the connection parameters.
            ESP_LOGI(TAG, "connection updated; status=%d",
                        event->conn_update.status);

            // Print connection descriptor
            response = ble_gap_conn_find(event->conn_update.conn_handle, &connectionDescription);
            if (response != 0) {
                ESP_LOGE(TAG, "failed to find connection by handle, error code: %d",
                            response);
                return response;
            }
            printConnectionDescription(&connectionDescription);
            return response;
    }
    return response;
}

int BleAdvertiser::mtuEventHandler(uint16_t conn_handle, const ble_gatt_error *error, uint16_t mtu, void *arg){
    ESP_LOGI(TAG, "MTU exchanged. MTU set to %d", mtu);

    // Set the MTU
    BleAdvertiser::mtu = mtu;

    return 0;
}

////////////////////////////////////////////////////////////////////////////
// Nimble stack event callback functions
////////////////////////////////////////////////////////////////////////////

void BleAdvertiser::onStackReset(int reason) {
    // On reset, print reset reason to console
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

void BleAdvertiser::onStackSync(void) {
    // This function will be called once the nimble host stack is synced with the BLE controller
    // Once the stack is synced, we can do advertizing initialization and begin advertising

    // Initialize and start advertizing
    initializeAdvertising();
}

void BleAdvertiser::gattSvrRegisterCb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    // Local variables to be reused
    char buf[BLE_UUID_STR_LEN];

    // Handle GATT attributes register events
    switch (ctxt->op) {

        // Service register event
        case BLE_GATT_REGISTER_OP_SVC:
            ESP_LOGD(TAG, "registered service %s with handle=%d",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
            break;

        // Characteristic register event
        case BLE_GATT_REGISTER_OP_CHR:
            ESP_LOGD(TAG,
                    "registering characteristic %s with "
                    "def_handle=%d val_handle=%d",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle, ctxt->chr.val_handle);
            break;

        // Descriptor register event
        case BLE_GATT_REGISTER_OP_DSC:
            ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
            break;

        // Unknown event
        default:
            assert(0);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
// Advertising helper functions
////////////////////////////////////////////////////////////////////////////

void BleAdvertiser::initializeAdvertising(void) {
    // Local variables to be reused
    int rc = 0;
    char addressString[18] = {0};

    // Make sure we have a proper BT address
    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "device does not have any available bt address!");
        return;
    }

    // Determine the BL address type to use while advertizing
    rc = ble_hs_id_infer_auto(0, &deviceAddressType);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to infer address type, error code: %d", rc);
        return;
    }

    // Copy the device address to deviceAddress
    rc = ble_hs_id_copy_addr(deviceAddressType, deviceAddress, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to copy device address, error code: %d", rc);
        return;
    }
    formatAddress(addressString, deviceAddress);
    ESP_LOGI(TAG, "device address: %s", addressString);

    // Start advertising.
    startAdvertising();
}

void BleAdvertiser::startAdvertising(void) {
    // Local variables to be reused
    int rc = 0;
    const char *name;
    struct ble_hs_adv_fields advertisingFields = {0};
    struct ble_hs_adv_fields responseFields = {0};
    struct ble_gap_adv_params advertisingParams = {0};

    // Set advertising flags
    advertisingFields.flags =
        BLE_HS_ADV_F_DISC_GEN | // advertising is general discoverable
        BLE_HS_ADV_F_BREDR_UNSUP; // BLE support only (BR/EDR refers to Bluetooth Classic)

    // Set device name
    name = ble_svc_gap_device_name(); // This was previously set in the GAP initialization step
    advertisingFields.name = (uint8_t *)name;
    advertisingFields.name_len = strlen(name);
    advertisingFields.name_is_complete = 1;

    // Set device tx power
    advertisingFields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    advertisingFields.tx_pwr_lvl_is_present = 1;

    // Set device appearance
    advertisingFields.appearance = deviceAppearance; // 4 bytes
    advertisingFields.appearance_is_present = 1;

    // Set device LE role
    advertisingFields.le_role = deviceRole; // 2 bytes
    advertisingFields.le_role_is_present = 1;

    /*
        It would be great if we could directly get the size of the advertizing packet,
        but unfortunately the function that makes this size available (adv_set_fields)
        appears to be private, though I could be wrong
    */
    // uint8_t buf[BLE_HS_ADV_MAX_SZ];
    // uint8_t buf_sz;
    // adv_set_fields(&advertisingFields, buf, &buf_sz, sizeof buf);
    // printf("len: %d", buf_sz);

    // Set advertisement fields
    rc = ble_gap_adv_set_fields(&advertisingFields);
    if (rc != 0) {
        if (rc == BLE_HS_EMSGSIZE) {
            ESP_LOGE(TAG, "failed to set advertising data, message data too long. Maximum advertizing packet size is %d", BLE_HS_ADV_MAX_SZ);
            return;
        }

        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return;
    }

    // Set device address
    responseFields.device_addr = deviceAddress;
    responseFields.device_addr_type = deviceAddressType;
    responseFields.device_addr_is_present = 1;

    // Set URI
    responseFields.uri = esp_uri;
    responseFields.uri_len = sizeof(esp_uri);

    // Set advertising interval in the response packet
    responseFields.adv_itvl = BLE_GAP_ADV_ITVL_MS(500); // unit of advertising interval is 0.625ms so we use this function to convert to ms
    responseFields.adv_itvl_is_present = 1; // unit of advertising interval is 0.625ms so we use this function to convert to ms

    // Set scan response fields
    rc = ble_gap_adv_rsp_set_fields(&responseFields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set scan response data, error code: %d", rc);
        return;
    }

    // Set connectable and general discoverable mode
    advertisingParams.conn_mode = BLE_GAP_CONN_MODE_UND;
    advertisingParams.disc_mode = BLE_GAP_DISC_MODE_GEN;

    // Set advertising interval in the advertizing packet
    advertisingParams.itvl_min = BLE_GAP_ADV_ITVL_MS(500); // unit of advertising interval is 0.625ms so we use this function to convert to ms
    advertisingParams.itvl_max = BLE_GAP_ADV_ITVL_MS(510); // unit of advertising interval is 0.625ms so we use this function to convert to ms

    // Start advertising
    rc = ble_gap_adv_start(deviceAddressType, NULL, BLE_HS_FOREVER, &advertisingParams,
                           gapEventHandler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "advertising started!");
}

////////////////////////////////////////////////////////////////////////////
// Service definition builder
////////////////////////////////////////////////////////////////////////////

esp_err_t BleAdvertiser::createGattServiceDefinitions() {
    // Get the number of services
    size_t servicesLength = services.size();

    // Allocate memory for the ble_gatt_svc_def array + 1 for the terminator
    // Note: This memory must be explicitly de-allocated which is done by the stop function of this class
    gattServiceDefinitions = (struct ble_gatt_svc_def*)malloc((servicesLength + 1) * sizeof(struct ble_gatt_svc_def));

    // Check if malloc was successful
    if (gattServiceDefinitions == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    // Create each service
    for (size_t index = 0; index < servicesLength; index++) {
        // Populate the characteristic definition
        ESP_ERROR_CHECK(services[index].get()->populateGattServiceDefinition(&gattServiceDefinitions[index]));
    }

    // Index the characteristics by their handles
    for (size_t index = 0; index < servicesLength; index++) {
        // Get the characteristics for the service
        vector<shared_ptr<BleCharacteristic>> characteristics = services[index].get()->characteristics;
        for (size_t characteristicIndex = 0; characteristicIndex < characteristics.size(); characteristicIndex++) {
            // Get the characteristic handle
            uint16_t* characteristicHandle = characteristics[characteristicIndex].get()->getHandle();

            // Index the characteristic by it's handle
            characteristicsByHandle[characteristicHandle] = characteristics[characteristicIndex];
        }
    }

    // Add the terminator { 0 } at the end
    gattServiceDefinitions[servicesLength] = (struct ble_gatt_svc_def){ 0 };

    return ESP_OK;
}
