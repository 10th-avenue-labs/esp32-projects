idf_component_register(
    SRCS "IDeserializable.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "ISerializable" "json" "Result" "AdtService"
)
