idf_component_register(
    SRCS "BleCharacteristic.cpp" "BleService.cpp" "BleAdvertiser.cpp" "BleDevice.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "bt" "nvs_flash"
)
