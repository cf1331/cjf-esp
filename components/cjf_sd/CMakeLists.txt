idf_component_register(SRCS "sd.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling esp_driver_gpio esp_driver_sdmmc esp_timer fatfs)
