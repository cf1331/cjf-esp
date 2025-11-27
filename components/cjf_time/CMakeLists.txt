idf_component_register(SRCS "sntp.cpp" "time.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES cjf_error_handling cjf_event cjf_nvs esp_common esp_netif)
