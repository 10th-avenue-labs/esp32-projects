idf_component_register(
    SRCS "WifiService.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "esp_wifi" "nvs_flash"
)
