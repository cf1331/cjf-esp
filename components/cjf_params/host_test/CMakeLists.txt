cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)  # Allow FetchContent deps with old cmake_minimum_required

project(params_tests CXX C)  # C required for cJSON.c

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── IDF_PATH ──────────────────────────────────────────────────────────────────
set(IDF_PATH "$ENV{IDF_PATH}" CACHE PATH
    "Path to ESP-IDF root. Defaults to the IDF_PATH environment variable.")

if(IDF_PATH STREQUAL "")
    message(FATAL_ERROR
        "IDF_PATH is not set.\n"
        "Either export IDF_PATH in your shell, or pass -DIDF_PATH=<path> to cmake.")
endif()

# On MSYS2/MinGW, convert Windows-style IDF_PATH (C:/...) to POSIX (/c/...)
# so that g++ receives valid absolute include paths.
find_program(CYGPATH cygpath)
if(CYGPATH)
    execute_process(
        COMMAND ${CYGPATH} -u "${IDF_PATH}"
        OUTPUT_VARIABLE IDF_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

message(STATUS "IDF_PATH: ${IDF_PATH}")

include(FetchContent)

# ── doctest ───────────────────────────────────────────────────────────────────
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(doctest)

# ── Component and cJSON source files ─────────────────────────────────────────
set(COMPONENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_executable(params_tests
    test_params.cpp
    test_mutable_param.cpp
    test_inplace_string_param.cpp
    ${COMPONENT_DIR}/params.cpp
    ${COMPONENT_DIR}/const_param.cpp
    ${COMPONENT_DIR}/json_serialization.cpp
    ${IDF_PATH}/components/json/cJSON/cJSON.c
)

# ── Include paths ─────────────────────────────────────────────────────────────
# IDF paths are listed first so the real esp_err.h and cJSON.h take priority.
# stubs/ provides:
#   - esp_log.h          (no-op logging macros)
#   - sdkconfig.h        (empty; satisfies esp_compiler.h -> sdkconfig.h)
#   - cjf/freertos/semaphore.h  (std::mutex-backed host implementation)
target_include_directories(params_tests PRIVATE
    ${IDF_PATH}/components/esp_common/include      # esp_err.h, esp_compiler.h
    ${IDF_PATH}/components/json/cJSON              # cJSON.h
    stubs                                          # esp_log.h, sdkconfig.h, cjf/freertos/semaphore.h
    ${COMPONENT_DIR}/include                       # cjf/params.h, cjf/watchable.h, ...
    ${COMPONENT_DIR}/../cjf_string/include         # cjf/string.h
    ${COMPONENT_DIR}/../cjf_type_traits/include    # cjf/type_traits.h
    ${COMPONENT_DIR}/../magic_enum/include         # magic_enum/magic_enum.hpp
)

target_link_libraries(params_tests PRIVATE
    doctest::doctest
)

# ── Warnings ──────────────────────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(params_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── CTest integration ─────────────────────────────────────────────────────────
include(CTest)
add_test(NAME params_tests COMMAND params_tests)
