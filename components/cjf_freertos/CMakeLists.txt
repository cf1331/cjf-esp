idf_component_register(SRCS "freertos_raii.cpp"
                            "task_controller.cpp"
                            "semaphore.cpp"
                            "event_group.cpp"
                            "timer.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES cjf_error_handling freertos)
