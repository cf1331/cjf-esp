idf_component_register(SRCS "mjpeg_middleware.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling cjf_web_server esp32-camera)
