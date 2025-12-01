idf_component_register(SRCS "wifi_mode_soft_ap.cpp" "wifi.cpp" "wifi_mode_smartconfig.cpp" "wifi_mode_sta.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling cjf_event cjf_nvs cjf_resource esp_common esp_netif esp_wifi magic_enum)
