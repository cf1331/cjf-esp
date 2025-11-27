idf_component_register(SRCS "nvs.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling cjf_resource nvs_flash)
