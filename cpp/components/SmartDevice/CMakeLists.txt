idf_component_register(
    SRCS "IDeserializable.cpp" "Message.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "ISerializable" "json" "Result" "AdtService" "Waiter"
)
