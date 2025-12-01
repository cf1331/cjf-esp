idf_component_register(SRCS "params.cpp" "const_param.cpp" "json_serialization.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_string freertos json magic_enum)
