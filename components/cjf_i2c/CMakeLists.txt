set(I2C_SLAVE_SRCS "i2c_device.cpp" "i2c_master.cpp")

if(CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2)
    set(I2C_SLAVE_SRCS ${I2C_SLAVE_SRCS} "i2c_slave.cpp")
endif()

idf_component_register(SRCS  ${I2C_SLAVE_SRCS}
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling cjf_resource esp_common esp_driver_i2c)
