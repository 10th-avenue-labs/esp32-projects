idf_component_register(
    SRCS "AcDimmer.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "esp_driver_gpio" "Timer"
)
