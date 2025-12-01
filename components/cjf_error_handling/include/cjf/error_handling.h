#ifndef F71A9988_B330_430E_A40E_9E585BBA5F2C
#define F71A9988_B330_430E_A40E_9E585BBA5F2C

#include <esp_check.h>
#include <esp_err.h>
#include <esp_log.h>
#include <exception>
#include <expected>
#include <source_location>

namespace cjf
{

/**
 * Helper macro to log an error with esp_err_t code.
 * Two-tier logging:
 * - No args: logs just error code and location
 * - With args: passes complete message as-is to ESP_LOGE (user provides format string and args)
 */
#define __LOG_ERROR_WITH_CODE(log_tag, err_code, ...)                                     \
  do                                                                                      \
  {                                                                                       \
    __VA_OPT__(ESP_LOGE(log_tag, __VA_ARGS__);)                                           \
    __VA_OPT__(if (0)) /* Skip else branch if args provided */                            \
    {                                                                                     \
      ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(err_code), __FUNCTION__, __LINE__); \
    }                                                                                     \
  } while (0)

/**
 * Helper macro to log an error with std::expected error code.
 * Extracts the error code and delegates to __LOG_ERROR_WITH_CODE.
 */
#define __LOG_ERROR_WITH_EXPECTED(log_tag, expected_val, ...) \
  __LOG_ERROR_WITH_CODE(log_tag, expected_val.error(), ##__VA_ARGS__)

/**
 * Macro to check an `esp_err_t` value and log an error message if it not `ESP_OK`.
 *
 * The error message will contain the function name, line number and error name.
 * An optional third argument can be provided to include a custom message.
 */
#define LOG_IF_ERROR(value, log_tag, ...)                     \
  do                                                          \
  {                                                           \
    esp_err_t err_rc_ = (value);                              \
    if (unlikely(err_rc_ != ESP_OK))                          \
    {                                                         \
      __LOG_ERROR_WITH_CODE(log_tag, err_rc_, ##__VA_ARGS__); \
    }                                                         \
  } while (0)

#define BREAK_ON_ERROR(value, log_tag, ...)                   \
  if (1)                                                      \
  {                                                           \
    esp_err_t err_rc_ = (value);                              \
    if (unlikely(err_rc_ != ESP_OK))                          \
    {                                                         \
      __LOG_ERROR_WITH_CODE(log_tag, err_rc_, ##__VA_ARGS__); \
      break;                                                  \
    }                                                         \
  }                                                           \
  else                                                        \
  {                                                           \
  }

#define BREAK_ON_UNEXPECTED(value, log_tag, ...)              \
  if (!value)                                                 \
  {                                                           \
    __LOG_ERROR_WITH_EXPECTED(log_tag, value, ##__VA_ARGS__); \
    break;                                                    \
  }                                                           \
  else                                                        \
  {                                                           \
  }

#define CONTINUE_ON_ERROR(value, log_tag, ...)                \
  if (1)                                                      \
  {                                                           \
    esp_err_t err_rc_ = (value);                              \
    if (unlikely(err_rc_ != ESP_OK))                          \
    {                                                         \
      __LOG_ERROR_WITH_CODE(log_tag, err_rc_, ##__VA_ARGS__); \
      continue;                                               \
    }                                                         \
  }                                                           \
  else                                                        \
  {                                                           \
  }

#define CONTINUE_ON_UNEXPECTED(value, log_tag, ...)           \
  if (!value)                                                 \
  {                                                           \
    __LOG_ERROR_WITH_EXPECTED(log_tag, value, ##__VA_ARGS__); \
    continue;                                                 \
  }                                                           \
  else                                                        \
  {                                                           \
  }

/**
 * Macro to check an `esp_err_t` value, log an error message and return the
 * value if it is not `ESP_OK`.
 *
 * The error message will contain the function name, line number and error name.
 * An optional third argument can be provided to include a custom message.
 * Modified from `ESP_RETURN_ON_ERROR` to include the error name in the log
 * message.
 */
#define CONTINUE_ON_UNEXPECTED_WITH_DELAY(value, delay_ms, log_tag, ...) \
  if (!value)                                                            \
  {                                                                      \
    __LOG_ERROR_WITH_EXPECTED(log_tag, value, ##__VA_ARGS__);            \
    vTaskDelay(pdMS_TO_TICKS(delay_ms));                                 \
    continue;                                                            \
  }                                                                      \
  else                                                                   \
  {                                                                      \
  }

/**
 * Macro to check an `esp_err_t` value, log an error message and return the
 * value if it is not `ESP_OK`.
 *
 * The error message will contain the function name, line number and error name.
 * An optional third argument can be provided to include a custom message.
 * Modified from `ESP_RETURN_ON_ERROR` to include the error name in the log
 * message.
 */
#define RETURN_ON_ERROR(value, log_tag, ...)                  \
  do                                                          \
  {                                                           \
    esp_err_t err_rc_ = (value);                              \
    if (unlikely(err_rc_ != ESP_OK))                          \
    {                                                         \
      __LOG_ERROR_WITH_CODE(log_tag, err_rc_, ##__VA_ARGS__); \
      return err_rc_;                                         \
    }                                                         \
  } while (0)

/**
 * Macro to check an `esp_err_t` value, log an error message and return the
 * value if it is not `ESP_OK`.
 *
 * The error message will contain the function name, line number and error name.
 * An optional third argument can be provided to include a custom message.
 * Modified from `ESP_RETURN_ON_ERROR` to include the error name in the log
 * message.
 */
#define RETURN_ON_UNEXPECTED(value, log_tag, ...)               \
  do                                                            \
  {                                                             \
    if (!value)                                                 \
    {                                                           \
      __LOG_ERROR_WITH_EXPECTED(log_tag, value, ##__VA_ARGS__); \
      return std::unexpected(value.error());                    \
    }                                                           \
  } while (0)

/**
 * Macro to check an `esp_err_t` value, log an error message and return the
 * value if it is not `ESP_OK`.
 *
 * The error message will contain the function name, line number and error name.
 * An optional third argument can be provided to include a custom message.
 * Modified from `ESP_RETURN_ON_ERROR` to include the error name in the log
 * message.
 */
#define RETURN_ERROR_ON_UNEXPECTED(value, log_tag, ...)         \
  do                                                            \
  {                                                             \
    if (!value)                                                 \
    {                                                           \
      __LOG_ERROR_WITH_EXPECTED(log_tag, value, ##__VA_ARGS__); \
      return value.error();                                     \
    }                                                           \
  } while (0)

/**
 * Macro to check an `esp_err_t` value, log an error message and return an
 * `std::unexpected<esp_err_t>` with the value if it is not `ESP_OK`.
 *
 * The error message will contain the function name, line number and error name.
 * An optional third argument can be provided to include a custom message.
 * Modified from `ESP_RETURN_ON_ERROR` to include the error name in the log
 * message.
 */
#define RETURN_UNEXPECTED_ON_ERROR(value, log_tag, ...)       \
  do                                                          \
  {                                                           \
    esp_err_t err_rc_ = (value);                              \
    if (unlikely(err_rc_ != ESP_OK))                          \
    {                                                         \
      __LOG_ERROR_WITH_CODE(log_tag, err_rc_, ##__VA_ARGS__); \
      return std::unexpected(err_rc_);                        \
    }                                                         \
  } while (0)

/**
 * Macro to check an `esp_err_t` value, log an error message and return the
 * void if it is not `ESP_OK`.
 *
 * The error message will contain the function name, line number and error name.
 * An optional third argument can be provided to include a custom message.
 * Modified from `ESP_RETURN_ON_ERROR` to include the error name in the log
 * message.
 */
#define RETURN_VOID_ON_ERROR(value, log_tag, ...)             \
  do                                                          \
  {                                                           \
    esp_err_t err_rc_ = (value);                              \
    if (unlikely(err_rc_ != ESP_OK))                          \
    {                                                         \
      __LOG_ERROR_WITH_CODE(log_tag, err_rc_, ##__VA_ARGS__); \
      return;                                                 \
    }                                                         \
  } while (0)

/**
 * Macro to check a `std::expected` value, log an error message and return
 * void if it contains an error.
 *
 * The error message will contain the function name, line number and error name.
 * An optional third argument can be provided to include a custom message.
 */
#define RETURN_VOID_ON_UNEXPECTED(value, log_tag, ...)          \
  do                                                            \
  {                                                             \
    if (!value)                                                 \
    {                                                           \
      __LOG_ERROR_WITH_EXPECTED(log_tag, value, ##__VA_ARGS__); \
      return;                                                   \
    }                                                           \
  } while (0)

/**
 * Macro to return an `std::unexpected` when a value is `false`.
 *
 * An optional fourth argument can be provided to log an error message.
 */
#define RETURN_UNEXPECTED_ON_FALSE(value, unexpected_value, log_tag, ...)  \
  do                                                                       \
  {                                                                        \
    if (unlikely(!(value)))                                                \
    {                                                                      \
      if (sizeof("" #__VA_ARGS__) > 1)                                     \
      {                                                                    \
        ESP_LOGE(log_tag, __VA_ARGS__ " (%s:%d)", __FUNCTION__, __LINE__); \
      }                                                                    \
      return std::unexpected(unexpected_value);                            \
    }                                                                      \
  } while (0)

/**
 * Macro to return an `esp_err_t` when a value is `false`.
 *
 * An optional third argument can be provided to log an error message.
 */
#define RETURN_ERROR_ON_FALSE(value, error_code, log_tag, ...)             \
  do                                                                       \
  {                                                                        \
    if (unlikely(!(value)))                                                \
    {                                                                      \
      if (sizeof("" #__VA_ARGS__) > 1)                                     \
      {                                                                    \
        ESP_LOGE(log_tag, __VA_ARGS__ " (%s:%d)", __FUNCTION__, __LINE__); \
      }                                                                    \
      return error_code;                                                   \
    }                                                                      \
  } while (0)

/**
 * Macro to continue a loop when a value is `false`.
 *
 * An optional third argument can be provided to log an error message.
 */
#define CONTINUE_ON_FALSE(value, log_tag, ...)                             \
  if (1)                                                                   \
  {                                                                        \
    if (unlikely(!(value)))                                                \
    {                                                                      \
      if (sizeof("" #__VA_ARGS__) > 1)                                     \
      {                                                                    \
        ESP_LOGE(log_tag, __VA_ARGS__ " (%s:%d)", __FUNCTION__, __LINE__); \
      }                                                                    \
      continue;                                                            \
    }                                                                      \
  }                                                                        \
  else                                                                     \
  {                                                                        \
  }

/**
 * Macro to break from a loop when a value is `false`.
 *
 * An optional third argument can be provided to log an error message.
 */
#define BREAK_ON_FALSE(value, log_tag, ...)                                \
  if (1)                                                                   \
  {                                                                        \
    if (unlikely(!(value)))                                                \
    {                                                                      \
      if (sizeof("" #__VA_ARGS__) > 1)                                     \
      {                                                                    \
        ESP_LOGE(log_tag, __VA_ARGS__ " (%s:%d)", __FUNCTION__, __LINE__); \
      }                                                                    \
      break;                                                               \
    }                                                                      \
  }                                                                        \
  else                                                                     \
  {                                                                        \
  }

} // namespace cjf

#endif /* F71A9988_B330_430E_A40E_9E585BBA5F2C */
