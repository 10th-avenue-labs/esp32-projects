#include "wifi_service.h"

#define DEFAULT_SCAN_LIST_SIZE 10

static char* TAG = "WIFI_SERVICE";

constexpr size_t maxSsidLength = sizeof(((wifi_sta_config_t*)0)->ssid) - 1;


// Define static members to satisfy the compiler
ConnectionState WifiService::connectionState = ConnectionState::NOT_CONNECTED;

static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_OWE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OWE");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA3_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_ENT_192:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_ENT_192");
        break;
    default:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

ConnectionState WifiService::getConnectionState() {
    return connectionState;
};

void WifiService::waitConnectionState(ConnectionState connectionState) {
    // TODO: Implement a timeout
    // TODO: Implement a way to cancel the wait
    // TODO: Use non-polling logic. i.e. event groups, tasks, semaphores, etc.
    while (WifiService::getConnectionState() != connectionState) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
};

bool WifiService::init() {
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

    // Initialize the TCP/IP network interface
    ESP_LOGI(TAG, "initializing tcp/ip network interface");
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_LOGI(TAG, "creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create the default WiFi station interface
    // Default event loop must be created before calling this function
    ESP_LOGI(TAG, "creating default wifi station interface");
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Initialize WiFi configuration with default values
    ESP_LOGI(TAG, "initializing wifi configuration");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Set the WiFi mode to station
    ESP_LOGI(TAG, "setting wifi mode to station");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Register an event handler to handle all WiFi events
    ESP_LOGI(TAG, "registering WIFI event handler");
    esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &genericEventHandler,
        NULL,
        &instance_any_id
    ));

    // Register an event handler to handle IP obtained events
    ESP_LOGI(TAG, "registering IP event handler");
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &genericEventHandler,
        NULL,
        &instance_got_ip
    ));

    return true;
};

ApScanResults WifiService::scanAvailableAccessPoints(uint8_t maxApsCount) {
    // Start the WiFi station
    ESP_LOGI(TAG, "starting wifi station");
    ESP_ERROR_CHECK(esp_wifi_start());

    // Scan for WiFi networks
    ESP_LOGI(TAG, "scanning for wifi networks");
    /**
     * Found APs are stored in WiFi driver dynamic allocated memory.
     * Can be freed in esp_wifi_scan_get_ap_records(), esp_wifi_scan_get_ap_record(), or esp_wifi_clear_ap_list().
     * 
     */
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));

    // Get the number of APs found
    uint16_t foundApsCount = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&foundApsCount));
    ESP_LOGI(TAG, "number of Access Points found = %u", foundApsCount);

    // Check if the number of APs found is greater than the max number of APs we can store
    uint16_t apsAllocated = foundApsCount;
    if (foundApsCount > maxApsCount) {
        ESP_LOGW(TAG, "found more access points than we can store, only storing %u", maxApsCount);
        apsAllocated = maxApsCount;
    }

    // Allocate memory for the APs
    wifi_ap_record_t* foundAps = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * apsAllocated);

    // Get the APs
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apsAllocated, foundAps));

    // Create a vector of AP scan results
    std::vector<ApScanResult> apScanResults;
    apScanResults.reserve(apsAllocated);

    // Populate the vector with the AP scan results
    for(int i = 0; i < apsAllocated; i++) {
        apScanResults.emplace_back(
            std::string(reinterpret_cast<char*>(foundAps[i].ssid), 32),
            foundAps[i].rssi,
            foundAps[i].authmode,
            foundAps[i].primary
        );
    }

    // Free the dynamically allocated memory for foundAps
    free(foundAps);

    return ApScanResults(foundApsCount, apScanResults);
};

bool WifiService::startConnect(
    ApCredentialInfo apCredentialInfo,
    std::function<void(void)> onConnect,
    std::function<void(void)> onDisconnect
) {
    // Check SSID length
    constexpr size_t maxSsidLength = sizeof(((wifi_sta_config_t*)0)->ssid) - 1;
    if (apCredentialInfo.ssid.length() > maxSsidLength) {
        ESP_LOGE(TAG, "SSID length is too long, max length is %zu", maxSsidLength);
        return false;
    }

    // Check password length
    constexpr size_t maxPasswordLength = sizeof(((wifi_sta_config_t*)0)->password) - 1;
    if (apCredentialInfo.password.length() > maxPasswordLength) {
        ESP_LOGE(TAG, "Password length is too long, max length is %zu", maxPasswordLength);
        return false;
    }

    // Initialize the WiFi configuration structure
    wifi_config_t wifi_config = {
        .sta = {
            .threshold = {
                .authmode = WIFI_AUTH_OPEN // This is the weakest authmode, effectively allowing any authmode
            },
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH, // Enable both Hunt and peck and hash to element
            .sae_h2e_identifier = "" // Not really sure what this is for, but it's only used for H2e
        }
    };

    // Copy SSID
    std::memset(wifi_config.sta.ssid, 0, sizeof(wifi_config.sta.ssid)); // Clear previous SSID (probably unnecessary with null-terminated strings)
    std::memcpy(wifi_config.sta.ssid, apCredentialInfo.ssid.c_str(), apCredentialInfo.ssid.length()); // Copy SSID
    wifi_config.sta.ssid[apCredentialInfo.ssid.length()] = '\0'; // Null-terminate SSID

    // Copy password
    std::memset(wifi_config.sta.password, 0, sizeof(wifi_config.sta.password)); // Clear previous password (probably unnecessary with null-terminated strings)
    std::memcpy(wifi_config.sta.password, apCredentialInfo.password.c_str(), apCredentialInfo.password.length()); // Copy password
    wifi_config.sta.password[apCredentialInfo.password.length()] = '\0'; // Null-terminate password

    // Set the WiFi configuration
    ESP_LOGI(TAG, "setting wifi configuration");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

    // Stop the wifi station
    /**
     * This logic here could be re-visited. Right now, we're stopping the wifi station in case it has already been started.
     * If it has already been started, the `WIFI_EVENT_STA_START` event would have already been triggered and wouldn't be triggered again.
     * This could happen if we've already connected to a network or already ran the scanAvailableAccessPoints function.
     * 
     * Doing things this way means we can avoid all that logic, however it comes at the cost of some efficiency.
     */
    ESP_LOGI(TAG, "stopping wifi station");
    ESP_ERROR_CHECK(esp_wifi_stop());

    // Set the connected state to connecting
    connectionState = ConnectionState::CONNECTING;

    // Start the WiFi station
    ESP_LOGI(TAG, "starting wifi station");
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_err_t response = esp_wifi_connect();
    if (response != ESP_OK) {
        ESP_LOGE(TAG, "failed to connect to WiFi, error code: %d", response);
        return false;
    }

    return false;
};

void WifiService::genericEventHandler(
    void* arg,                     // Argument to pass to the handler
    esp_event_base_t eventBase,    // The base event, eg a wifi event
    int32_t eventId,               // The specific event, eg wifi ready
    void* eventData                // Data associated with the event, like an IP address for a IP_EVENT_STA_GOT_IP event ID
) {
    ESP_LOGI(TAG, "genericEventHandler called with event_base: %s, event_id: %ld", eventBase, eventId);

    static const std::unordered_map<std::string, std::function<void(
        void* arg,
        int32_t eventId,
        void* eventData
    )>> handlers = {
        {WIFI_EVENT, wifiEventHandler},
        {IP_EVENT, ipEventHandler}
    };

    // Find the appropriate event handler for the event base
    auto iterator = handlers.find(eventBase);
    if (iterator == handlers.end()) {
        ESP_LOGE(TAG, "no handler found for event_base: %s", eventBase);
        return;
    }

    // Delegate the event to the appropriate handler
    iterator->second(
        arg,
        eventId,
        eventData
    );
};

void WifiService::wifiEventHandler(
    void* arg,                     // Argument to pass to the handler
    int32_t eventId,               // The specific event, eg wifi ready
    void* eventData                // Data associated with the event, like an IP address for a IP_EVENT_STA_GOT_IP event ID
) {
    ESP_LOGI(TAG, "wifiEventHandler called with event_id: %ld", eventId);

    switch(eventId) {
        case WIFI_EVENT_STA_START: {
            ESP_LOGI(TAG, "WiFi station started");
            // esp_err_t response = esp_wifi_connect();
            // if (response != ESP_OK) {
            //     ESP_LOGE(TAG, "failed to connect to WiFi, error code: %d", response);
            //     return;
            // }

            // TODO: Add delegate here


            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WiFi station disconnected");

            // Set the connection state to not connected
            connectionState = ConnectionState::NOT_CONNECTED;

            // TODO: Add delegate here
            break;
        default:
            ESP_LOGW(TAG, "Unhandled WiFi event: %ld", eventId);
            break;
    }
};

void WifiService::ipEventHandler(
    void* arg,                     // Argument to pass to the handler
    int32_t eventId,               // The specific event, eg wifi ready
    void* eventData                // Data associated with the event, like an IP address for a IP_EVENT_STA_GOT_IP event ID
) {
    ESP_LOGI(TAG, "ipEventHandler called with event_id: %ld", eventId);

    switch(eventId) {
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) eventData;
            ESP_LOGI(TAG, "IP address obtained: " IPSTR, IP2STR(&event->ip_info.ip));

            // Set the connection state to connected
            connectionState = ConnectionState::CONNECTED;

            // TODO: Add delegate here
            break;
    }
}