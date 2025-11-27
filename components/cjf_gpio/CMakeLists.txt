idf_component_register(SRCS "gpio.cpp" "atomic_gpio.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling cjf_resource esp_common esp_driver_gpio)
