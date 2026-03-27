idf_component_register(SRCS "sntp.cpp" "time.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES cjf_error_handling cjf_event cjf_nvs cjf_resource esp_common esp_netif)
