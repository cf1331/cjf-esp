idf_component_register(SRCS "event_monitor.cpp" "modes.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling cjf_event cjf_freertos cjf_led_strip esp_common)
