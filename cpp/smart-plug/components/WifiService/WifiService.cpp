#include "WifiService.h"

static char *TAG = "WIFI_SERVICE";

// Define static members to satisfy the compiler
std::function<void(void)> WifiService::onConnect = nullptr;
std::function<void(void)> WifiService::onDisconnect = nullptr;
std::atomic<ConnectionState> WifiService::connectionState = ConnectionState::NOT_CONNECTED;
std::condition_variable WifiService::stateChanged = std::condition_variable();
std::mutex WifiService::stateMutex = std::mutex();

////////////////////////////////////////////////////////////////////////////////
// Initialization and Disposal
////////////////////////////////////////////////////////////////////////////////

bool WifiService::init()
{
    // Initialise the non-volatile flash storage (NVS)
    ESP_LOGI(TAG, "initializing nvs flash");
    esp_err_t response = nvs_flash_init();

    // Attempt to recover if an error occurred
    if (response != ESP_OK)
    {
        // Check if a recoverable error occured
        if (response == ESP_ERR_NVS_NO_FREE_PAGES ||
            response == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {

            // Erase and re-try if necessary
            // Note: This will erase the nvs flash.
            // TODO: We should consider alternative impls here. Erasing the NVS could be a very unwanted side-effect
            ESP_LOGI(TAG, "erasing nvs flash");
            ESP_ERROR_CHECK(nvs_flash_erase());
            response = nvs_flash_init();

            if (response != ESP_OK)
            {
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
        &instance_any_id));

    // Register an event handler to handle IP obtained events
    ESP_LOGI(TAG, "registering IP event handler");
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &genericEventHandler,
        NULL,
        &instance_got_ip));

    return true;
};

// TODO: Implement disposal logic

////////////////////////////////////////////////////////////////////////////////
// Access Point Scanning
////////////////////////////////////////////////////////////////////////////////

ApScanResults WifiService::scanAvailableAccessPoints(uint8_t maxApsCount)
{
    // Ensure we are not in the process of connecting or disconnecting
    if (getConnectionState() == ConnectionState::CONNECTING || getConnectionState() == ConnectionState::DISCONNECTING)
    {
        ESP_LOGE(TAG, "cannot scan for access points while connecting or disconnecting");
        return ApScanResults(0, {});
    }

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
    if (foundApsCount > maxApsCount)
    {
        ESP_LOGW(TAG, "found more access points than we can store, only storing %u", maxApsCount);
        apsAllocated = maxApsCount;
    }

    // Allocate memory for the APs
    wifi_ap_record_t *foundAps = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apsAllocated);

    // Get the APs
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apsAllocated, foundAps));

    // Create a vector of AP scan results
    std::vector<ApScanResult> apScanResults;
    apScanResults.reserve(apsAllocated);

    // Populate the vector with the AP scan results
    for (int i = 0; i < apsAllocated; i++)
    {
        apScanResults.emplace_back(
            std::string(reinterpret_cast<char *>(foundAps[i].ssid), 32),
            foundAps[i].rssi,
            foundAps[i].authmode,
            foundAps[i].primary);
    }

    // Free the dynamically allocated memory for foundAps
    free(foundAps);

    return ApScanResults(foundApsCount, apScanResults);
};

////////////////////////////////////////////////////////////////////////////////
// Connection Management
////////////////////////////////////////////////////////////////////////////////

bool WifiService::startConnect(
    ApCredentialInfo apCredentialInfo)
{
    // Check the connection state
    switch (getConnectionState())
    {
    case ConnectionState::NOT_CONNECTED:
        // valid state
        break;
    case ConnectionState::CONNECTING:
        ESP_LOGE(TAG, "cannot connect to an access point while already connecting");
        return false;
    case ConnectionState::CONNECTED:
        ESP_LOGE(TAG, "cannot connect to an access point while already connected");
        return false;
    case ConnectionState::DISCONNECTING:
        ESP_LOGE(TAG, "cannot connect to an access point while already disconnecting");
        return false;
    default:
        ESP_LOGE(TAG, "unknown connection state");
        return false;
    }

    // Check SSID length
    constexpr size_t maxSsidLength = sizeof(((wifi_sta_config_t *)0)->ssid) - 1;
    if (apCredentialInfo.ssid.length() > maxSsidLength)
    {
        ESP_LOGE(TAG, "SSID length is too long, max length is %zu", maxSsidLength);
        return false;
    }

    // Check password length
    constexpr size_t maxPasswordLength = sizeof(((wifi_sta_config_t *)0)->password) - 1;
    if (apCredentialInfo.password.length() > maxPasswordLength)
    {
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
            .sae_h2e_identifier = ""          // Not really sure what this is for, but it's only used for H2e
        }};

    // Copy SSID
    std::memcpy(wifi_config.sta.ssid, apCredentialInfo.ssid.c_str(), apCredentialInfo.ssid.length());

    // Copy password
    std::memcpy(wifi_config.sta.password, apCredentialInfo.password.c_str(), apCredentialInfo.password.length());

    // Set the WiFi configuration
    ESP_LOGI(TAG, "setting wifi configuration");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

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
    setConnectionState(ConnectionState::CONNECTING);

    // Start the WiFi station
    ESP_LOGI(TAG, "starting wifi station");
    ESP_ERROR_CHECK(esp_wifi_start());

    // Connect to the WiFi network
    ESP_LOGI(TAG, "connecting to wifi network");
    esp_err_t response = esp_wifi_connect();
    if (response != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to connect to WiFi, error code: %d", response);
        return false;
    }

    return true;
};

bool WifiService::startDisconnect()
{
    // Check the connection state
    if (getConnectionState() != ConnectionState::CONNECTED)
    {
        ESP_LOGW(TAG, "cannot disconnect from an access point while not connected");
        return false;
    }

    // Set the connection state to disconnecting
    setConnectionState(ConnectionState::DISCONNECTING);

    // Stop the wifi station
    ESP_LOGI(TAG, "stopping wifi station");
    ESP_ERROR_CHECK(esp_wifi_stop());

    return true;
};

////////////////////////////////////////////////////////////////////////////////
// Connection State Management
////////////////////////////////////////////////////////////////////////////////

ConnectionState WifiService::getConnectionState()
{
    return connectionState.load(std::memory_order_relaxed);
}

void WifiService::setConnectionState(ConnectionState newState)
{
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        connectionState.store(newState, std::memory_order_relaxed);
    }

    // Notify any waiting threads
    stateChanged.notify_all();
}

bool WifiService::waitConnectionState(const std::vector<ConnectionState> &connectionStates, int timeoutMs)
{
    // TODO: Implement a way to cancel the wait

    std::unique_lock<std::mutex> lock(stateMutex);

    // Define a lambda to check if current state is in connectionStates
    auto isDesiredState = [&]()
    {
        return std::find(connectionStates.begin(), connectionStates.end(), getConnectionState()) != connectionStates.end();
    };

    if (timeoutMs < 0)
    {
        // Wait indefinitely until the state matches one of the desired states
        stateChanged.wait(lock, isDesiredState);
        return true;
    }
    else
    {
        // Wait with timeout
        return stateChanged.wait_for(lock, std::chrono::milliseconds(timeoutMs), isDesiredState);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Event Handlers
////////////////////////////////////////////////////////////////////////////////

void WifiService::genericEventHandler(
    void *arg,
    esp_event_base_t eventBase,
    int32_t eventId,
    void *eventData)
{
    ESP_LOGI(TAG, "genericEventHandler called with event_base: %s, event_id: %ld", eventBase, eventId);

    static const std::unordered_map<std::string, std::function<void(
                                                     int32_t eventId,
                                                     void *eventData)>>
        handlers = {
            {WIFI_EVENT, wifiEventHandler},
            {IP_EVENT, ipEventHandler}};

    // Find the appropriate event handler for the event base
    auto iterator = handlers.find(eventBase);
    if (iterator == handlers.end())
    {
        ESP_LOGE(TAG, "no handler found for event_base: %s", eventBase);
        return;
    }

    // Delegate the event to the appropriate handler
    iterator->second(
        eventId,
        eventData);
};

void WifiService::wifiEventHandler(
    int32_t eventId,
    void *eventData)
{
    ESP_LOGI(TAG, "wifiEventHandler called with event_id: %ld", eventId);

    switch (eventId)
    {
    case WIFI_EVENT_STA_START:
    {
        ESP_LOGI(TAG, "WiFi station started");
        break;
    }
    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "WiFi station disconnected");

        // Set the connection state to not connected
        setConnectionState(ConnectionState::NOT_CONNECTED);

        // Call the onDisconnect delegate
        if (onDisconnect != nullptr)
        {
            onDisconnect();
        }
        break;
    default:
        ESP_LOGW(TAG, "Unhandled WiFi event: %ld", eventId);
        break;
    }
};

void WifiService::ipEventHandler(
    int32_t eventId,
    void *eventData)
{
    ESP_LOGI(TAG, "ipEventHandler called with event_id: %ld", eventId);

    switch (eventId)
    {
    case IP_EVENT_STA_GOT_IP:
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)eventData;
        ESP_LOGI(TAG, "IP address obtained: " IPSTR, IP2STR(&event->ip_info.ip));

        // Set the connection state to connected
        setConnectionState(ConnectionState::CONNECTED);

        // Call the onConnect delegate
        if (onConnect != nullptr)
        {
            onConnect();
        }
        break;
    }
}
