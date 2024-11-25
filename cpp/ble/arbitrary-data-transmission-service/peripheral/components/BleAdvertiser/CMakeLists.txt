idf_component_register(
    SRCS "BleAdvertiser.cpp" "BleService.cpp" "BleCharacteristic.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "bt" "nvs_flash"
)
