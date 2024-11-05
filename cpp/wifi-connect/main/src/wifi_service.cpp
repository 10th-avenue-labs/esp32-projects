#include "wifi_service.h"

static char* TAG = "WIFI_SERVICE";

constexpr size_t maxSsidLength = sizeof(((wifi_sta_config_t*)0)->ssid) - 1;

bool WifiService::init(std::string ssid, std::string password) {
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

    // Skip this for right now, I don't think we'll need it
    // // Create the FreeRTOS event group
    // s_wifi_event_group = xEventGroupCreate();

    // Initialize the TCP/IP network interface
    ESP_LOGI(TAG, "initializing tcp/ip network interface");
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_LOGI(TAG, "creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create the default WiFi station interface
    // Default event loop must be created before calling this function
    ESP_LOGI(TAG, "creating default wifi station interface");
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi configuration with default values
    ESP_LOGI(TAG, "initializing wifi configuration");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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

    // Check SSID length
    constexpr size_t maxSsidLength = sizeof(((wifi_sta_config_t*)0)->ssid) - 1;
    if (ssid.length() > maxSsidLength) {
        ESP_LOGE(TAG, "SSID length is too long, max length is %zu", maxSsidLength);
        return false;
    }

    // Check password length
    constexpr size_t maxPasswordLength = sizeof(((wifi_sta_config_t*)0)->password) - 1;
    if (password.length() > maxPasswordLength) {
        ESP_LOGE(TAG, "Password length is too long, max length is %zu", maxPasswordLength);
        return false;
    }

    // Initialize the WiFi configuration structure
    wifi_config_t wifi_config = {
        .sta = {
            .threshold = {
                .authmode = WIFI_AUTH_OPEN // This is the weakest authmode, effectively allowing any authmode
            },
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_h2e_identifier = ""
        }
    };

    // Copy SSID
    std::memset(wifi_config.sta.ssid, 0, sizeof(wifi_config.sta.ssid)); // Clear previous SSID (probably unnecessary with null-terminated strings)
    std::memcpy(wifi_config.sta.ssid, ssid.c_str(), ssid.length()); // Copy SSID
    wifi_config.sta.ssid[ssid.length()] = '\0'; // Null-terminate SSID

    // Copy password
    std::memset(wifi_config.sta.password, 0, sizeof(wifi_config.sta.password)); // Clear previous password (probably unnecessary with null-terminated strings)
    std::memcpy(wifi_config.sta.password, password.c_str(), password.length()); // Copy password
    wifi_config.sta.password[password.length()] = '\0'; // Null-terminate password

    // Set the WiFi mode to station
    ESP_LOGI(TAG, "setting wifi mode to station");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );

    // Set the WiFi configuration
    ESP_LOGI(TAG, "setting wifi configuration");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

    // Start the WiFi station
    ESP_LOGI(TAG, "starting wifi station");
    ESP_ERROR_CHECK(esp_wifi_start());

    return false;
}

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
    printf("wifiEventHandler called with event_id: %ld\n", eventId);

    switch(eventId) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi station started");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WiFi station disconnected");
            break;
    }
};

void WifiService::ipEventHandler(
    void* arg,                     // Argument to pass to the handler
    int32_t eventId,               // The specific event, eg wifi ready
    void* eventData                // Data associated with the event, like an IP address for a IP_EVENT_STA_GOT_IP event ID
) {
    printf("ipEventHandler called with event_id: %ld\n", eventId);

    switch(eventId) {
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) eventData;
            ESP_LOGI(TAG, "IP address obtained:" IPSTR, IP2STR(&event->ip_info.ip));
            break;
    }
}