idf_component_register(SRCS "params.cpp" "const_param.cpp" "json_serialization.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_string cjf_type_traits cjf_freertos json magic_enum)
