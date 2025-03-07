/* Includes */
/* STD APIs */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* ESP APIs */
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

/* FreeRTOS APIs */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

/* NimBLE GAP APIs */
#include "services/gap/ble_svc_gap.h"

/* Defines */
#define TAG "NimBLE_Beacon"
#define DEVICE_NAME "Montana's_Beacon"
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_URI_PREFIX_HTTPS 0x17
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

/* Library function declarations */
void ble_store_config_init(void); // For some reason we need to manually declare this one? Not sure why as it lives in a library

static uint8_t own_addr_type;
static uint8_t addr_val[6] = {0};
static uint8_t esp_uri[] = {BLE_GAP_URI_PREFIX_HTTPS, '/', '/', 'e', 's', 'p', 'r', 'e', 's', 's', 'i', 'f', '.', 'c', 'o', 'm'};

// Function signatures
void adv_init(void);
inline static void format_addr(char *addr_str, uint8_t addr[]);
static void start_advertising(void);
static int gap_event_handler(struct ble_gap_event *event, void *arg);

// Print the connection description
static void print_conn_desc(struct ble_gap_conn_desc *desc) {
    // Local variables to be reused
    char addr_str[18] = {0};

    // Connection handle
    ESP_LOGI(TAG, "connection handle: %d", desc->conn_handle);

    // Local ID address
    format_addr(addr_str, desc->our_id_addr.val);
    ESP_LOGI(TAG, "device id address: type=%d, value=%s",
             desc->our_id_addr.type, addr_str);

    // Peer ID address
    format_addr(addr_str, desc->peer_id_addr.val);
    ESP_LOGI(TAG, "peer id address: type=%d, value=%s", desc->peer_id_addr.type,
             addr_str);

    // Connection info
    ESP_LOGI(TAG,
             "conn_itvl=%d, conn_latency=%d, supervision_timeout=%d, "
             "encrypted=%d, authenticated=%d, bonded=%d\n",
             desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
             desc->sec_state.encrypted, desc->sec_state.authenticated,
             desc->sec_state.bonded);
}

// Initialize the Generic Attribute Profile (GAP)
int gap_init(void) {
    // Local variables to be reused
    int rc = 0;

    // Initialize the gap service
    ble_svc_gap_init();

    // Set the gap device name
    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device name to %s, error code: %d",
                 DEVICE_NAME, rc);
        return rc;
    }

    // Set the gap appearance
    rc = ble_svc_gap_device_appearance_set(BLE_GAP_APPEARANCE_GENERIC_TAG);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device appearance, error code: %d", rc);
        return rc;
    }
    return rc;
}

/* Private functions */
/*
 *  Stack event callback functions
 *      - on_stack_reset is called when host resets BLE stack due to errors
 *      - on_stack_sync is called when host has synced with controller
 */
static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
    // This function will be called once the nimble host stack is synced with the BLE controller
    // Once the stack is synced, we can do advertizing initialization and begin advertising

    // Initialize and start advertizing
    adv_init();
}

// Initialize and start advertizing
void adv_init(void) {
    // Local variables to be reused
    int rc = 0;
    char addr_str[18] = {0};

    // Make sure we have a proper BT address
    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "device does not have any available bt address!");
        return;
    }

    // Determine the BL address type to use while advertizing
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to infer address type, error code: %d", rc);
        return;
    }

    // Copy the device address to addr_val
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to copy device address, error code: %d", rc);
        return;
    }
    format_addr(addr_str, addr_val);
    ESP_LOGI(TAG, "device address: %s", addr_str);

    // Start advertising.
    start_advertising();
}

inline static void format_addr(char *addr_str, uint8_t addr[]) {
    sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1],
            addr[2], addr[3], addr[4], addr[5]);
}

// Start advertizing
static void start_advertising(void) {
    // Local variables to be reused
    int rc = 0;
    const char *name;
    struct ble_hs_adv_fields adv_fields = {0};
    struct ble_hs_adv_fields rsp_fields = {0};
    struct ble_gap_adv_params adv_params = {0};

    // Set advertising flags
    adv_fields.flags =
        BLE_HS_ADV_F_DISC_GEN | // advertising is general discoverable
        BLE_HS_ADV_F_BREDR_UNSUP; // BLE support only (BR/EDR refers to Bluetooth Classic)

    // Set device name
    name = ble_svc_gap_device_name(); // This was previously set in the GAP initialization step
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;

    // Set device tx power
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    // Set device appearance
    adv_fields.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG; // 4 bytes
    adv_fields.appearance_is_present = 1;

    // Set device LE role
    adv_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL; // 2 byte
    adv_fields.le_role_is_present = 1;

    /*
        It would be great if we could directly get the size of the advertizing packet,
        but unfortunately the function that makes this size available (adv_set_fields)
        appears to be private, though I could be wrong
    */
    // uint8_t buf[BLE_HS_ADV_MAX_SZ];
    // uint8_t buf_sz;
    // adv_set_fields(&adv_fields, buf, &buf_sz, sizeof buf);
    // printf("len: %d", buf_sz);

    // Set advertizement fields
    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        if (rc == BLE_HS_EMSGSIZE) {
            ESP_LOGE(TAG, "failed to set advertising data, message data too long. Maximum advertizing packet size is %d", BLE_HS_ADV_MAX_SZ);
            return;
        }

        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return;
    }

    // Set device address
    rsp_fields.device_addr = addr_val;
    rsp_fields.device_addr_type = own_addr_type;
    rsp_fields.device_addr_is_present = 1;

    // Set URI
    rsp_fields.uri = esp_uri;
    rsp_fields.uri_len = sizeof(esp_uri);

    // Set advertising interval in the response packet
    rsp_fields.adv_itvl = BLE_GAP_ADV_ITVL_MS(500); // unit of advertising interval is 0.625ms so we use this function to convert to ms
    rsp_fields.adv_itvl_is_present = 1; // unit of advertising interval is 0.625ms so we use this function to convert to ms

    // Set scan response fields
    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set scan response data, error code: %d", rc);
        return;
    }

    // Set connectable and general discoverable mode
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    // Set advertising interval in the advertizing packet
    adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(500); // unit of advertising interval is 0.625ms so we use this function to convert to ms
    adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(510); // unit of advertising interval is 0.625ms so we use this function to convert to ms

    // Start advertising
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                           gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "advertising started!");
}

// Turn off LED
static void led_off() {
    // Turn off led
}

// Turn on LED
static void led_on() {
    // Turn on led
}

// GAP connection event handler
static int gap_event_handler(struct ble_gap_event *event, void *arg) {
    // Local variables to be reused
    int rc = 0;
    struct ble_gap_conn_desc desc;

    // Handle different GAP events
    switch (event->type) {
        // Connect event
        case BLE_GAP_EVENT_CONNECT:
            // A new connection was established or a connection attempt failed.
            ESP_LOGI(TAG, "connection %s; status=%d",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);

            // Connection succeeded
            if (event->connect.status == 0) {
                // Check connection handle
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                if (rc != 0) {
                    ESP_LOGE(TAG,
                            "failed to find connection by handle, error code: %d",
                            rc);
                    return rc;
                }

                // Print connection descriptor and turn on the LED
                print_conn_desc(&desc);
                led_on();

                // Try to update connection parameters
                struct ble_gap_upd_params params = {.itvl_min = desc.conn_itvl,
                                                    .itvl_max = desc.conn_itvl,
                                                    .latency = 3,
                                                    .supervision_timeout =
                                                        desc.supervision_timeout};
                rc = ble_gap_update_params(event->connect.conn_handle, &params);
                if (rc != 0) {
                    ESP_LOGE(
                        TAG,
                        "failed to update connection parameters, error code: %d",
                        rc);
                    return rc;
                }
            }
            // Connection failed, restart advertising
            else {
                start_advertising();
            }
            return rc;

        // Disconnect event
        case BLE_GAP_EVENT_DISCONNECT:
            // A connection was terminated, print connection descriptor
            ESP_LOGI(TAG, "disconnected from peer; reason=%d",
                    event->disconnect.reason);

            // Turn off the LED
            led_off();

            // Restart advertising
            start_advertising();
            return rc;

        // Connection parameters update event
        case BLE_GAP_EVENT_CONN_UPDATE:
            // The central has updated the connection parameters.
            ESP_LOGI(TAG, "connection updated; status=%d",
                        event->conn_update.status);

            // Print connection descriptor
            rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
            if (rc != 0) {
                ESP_LOGE(TAG, "failed to find connection by handle, error code: %d",
                            rc);
                return rc;
            }
            print_conn_desc(&desc);
            return rc;
    }
    return rc;
}

// Initialize the nimble host config
static void nimble_host_config_init(void) {
    // Set the host callbacks
    // Idk where these variables are coming from?
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    ble_store_config_init();
}

// Start the nimble host task
static void nimble_host_task(void *param) {
    // Log the start of the task
    ESP_LOGI(TAG, "nimble host task has been started!");

    // This function won't return until nimble_port_stop() is executed
    nimble_port_run();

    // Clean up at exit
    vTaskDelete(NULL);
}

void app_main(void) {
    // Local variables to be reused
    int rc;
    esp_err_t ret;

    // Initialise the non-volatile flash storage (NVS)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        // Erase and re-try if necessary
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    // Verify the response
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
        return;
    }

    // Initialise the controller and nimble host stack 
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ",
                 ret);
        return;
    }

    // Initialize the Generic Attribute Profile (GAP)
    rc = gap_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
        return;
    }

    // Initialize the nimble host configuration
    nimble_host_config_init();

    // Start NimBLE host task thread and return
    xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);
    ESP_LOGI(TAG, "Exiting main thread");
    return;
}