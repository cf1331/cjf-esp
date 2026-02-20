idf_component_register(SRCS "i2c_device.cpp" "i2c_master.cpp" "i2c_slave.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling cjf_resource esp_common esp_driver_i2c)
