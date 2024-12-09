idf_component_register(
    SRCS "MqttConfig.cpp" "BleConfig.cpp" "WifiConfig.cpp" "AcDimmerConfig.cpp" "PlugConfig.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "nvs_flash" "json" "Result"
)
