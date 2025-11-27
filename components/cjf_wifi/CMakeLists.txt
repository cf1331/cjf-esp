idf_component_register(SRCS "wifi.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling cjf_nvs cjf_resource esp_common esp_netif esp_wifi)
