idf_component_register(SRCS "web_server.cpp" "log_request_middleware.cpp" "multipart_stream_middleware.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling cjf_freertos cjf_memory cjf_timeout esp_http_server esp_netif esp_timer json log)
