idf_component_register(
    SRCS "IDeserializable.cpp" "Request.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "ISerializable" "json" "Result" "AdtService" "Waiter" "WifiService" "BleAdvertiser" "MqttClient"
)
