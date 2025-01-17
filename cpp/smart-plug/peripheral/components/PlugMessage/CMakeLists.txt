idf_component_register(
    SRCS "CloudConnectionInfo.cpp" "WifiConnectionInfo.cpp" "SetMqttConfig.cpp" "SetBleConfig.cpp" "SetAcDimmerConfig.cpp" "PlugMessage.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "json" "Result" "ISerializable"
)
