idf_component_register(
    SRCS "SetMqttConfig.cpp" "SetWifiConfig.cpp" "SetBleConfig.cpp" "SetAcDimmerConfig.cpp" "PlugMessage.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "json"
)
