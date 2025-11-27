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
 * Macro to check an `esp_err_t` value and log an error message if it not `ESP_OK`.
 *
 * The error message will contain the function name, line number and error name.
 * An optional third argument can be provided to include a custom message.
 */
#define LOG_IF_ERROR(value, log_tag, ...)                                                                \
  do                                                                                                     \
  {                                                                                                      \
    esp_err_t err_rc_ = (value);                                                                         \
    if (unlikely(err_rc_ != ESP_OK))                                                                     \
    {                                                                                                    \
      if (sizeof("" #__VA_ARGS__) > 1)                                                                   \
      {                                                                                                  \
        ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__); \
      }                                                                                                  \
      else                                                                                               \
      {                                                                                                  \
        ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__);               \
      }                                                                                                  \
    }                                                                                                    \
  } while (0)

#define BREAK_ON_ERROR(value, log_tag, ...)                                                              \
  if (1)                                                                                                 \
  {                                                                                                      \
    esp_err_t err_rc_ = (value);                                                                         \
    if (unlikely(err_rc_ != ESP_OK))                                                                     \
    {                                                                                                    \
      if (sizeof("" #__VA_ARGS__) > 1)                                                                   \
      {                                                                                                  \
        ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__); \
      }                                                                                                  \
      else                                                                                               \
      {                                                                                                  \
        ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__);               \
      }                                                                                                  \
      break;                                                                                             \
    }                                                                                                    \
  }                                                                                                      \
  else                                                                                                   \
  {                                                                                                      \
  }

#define BREAK_ON_UNEXPECTED(value, log_tag, ...)                                                             \
  if (!value)                                                                                                \
  {                                                                                                          \
    if (sizeof("" #__VA_ARGS__) > 1)                                                                         \
    {                                                                                                        \
      ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__); \
    }                                                                                                        \
    else                                                                                                     \
    {                                                                                                        \
      ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__);               \
    }                                                                                                        \
    break;                                                                                                   \
  }                                                                                                          \
  else                                                                                                       \
  {                                                                                                          \
  }

#define CONTINUE_ON_ERROR(value, log_tag, ...)                                                           \
  if (1)                                                                                                 \
  {                                                                                                      \
    esp_err_t err_rc_ = (value);                                                                         \
    if (unlikely(err_rc_ != ESP_OK))                                                                     \
    {                                                                                                    \
      if (sizeof("" #__VA_ARGS__) > 1)                                                                   \
      {                                                                                                  \
        ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__); \
      }                                                                                                  \
      else                                                                                               \
      {                                                                                                  \
        ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__);               \
      }                                                                                                  \
      continue;                                                                                          \
    }                                                                                                    \
  }                                                                                                      \
  else                                                                                                   \
  {                                                                                                      \
  }

#define CONTINUE_ON_UNEXPECTED(value, log_tag, ...)                                                          \
  if (!value)                                                                                                \
  {                                                                                                          \
    if (sizeof("" #__VA_ARGS__) > 1)                                                                         \
    {                                                                                                        \
      ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__); \
    }                                                                                                        \
    else                                                                                                     \
    {                                                                                                        \
      ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__);               \
    }                                                                                                        \
    continue;                                                                                                \
  }                                                                                                          \
  else                                                                                                       \
  {                                                                                                          \
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
#define CONTINUE_ON_UNEXPECTED_WITH_DELAY(value, delay_ms, log_tag, ...)                                     \
  if (!value)                                                                                                \
  {                                                                                                          \
    if (sizeof("" #__VA_ARGS__) > 1)                                                                         \
    {                                                                                                        \
      ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__); \
    }                                                                                                        \
    else                                                                                                     \
    {                                                                                                        \
      ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__);               \
    }                                                                                                        \
    vTaskDelay(pdMS_TO_TICKS(delay_ms));                                                                     \
    continue;                                                                                                \
  }                                                                                                          \
  else                                                                                                       \
  {                                                                                                          \
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
#define RETURN_ON_ERROR(value, log_tag, ...)                                                             \
  do                                                                                                     \
  {                                                                                                      \
    esp_err_t err_rc_ = (value);                                                                         \
    if (unlikely(err_rc_ != ESP_OK))                                                                     \
    {                                                                                                    \
      if (sizeof("" #__VA_ARGS__) > 1)                                                                   \
      {                                                                                                  \
        ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__); \
      }                                                                                                  \
      else                                                                                               \
      {                                                                                                  \
        ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__);               \
      }                                                                                                  \
      return err_rc_;                                                                                    \
    }                                                                                                    \
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
#define RETURN_ON_UNEXPECTED(value, log_tag, ...)                                                              \
  do                                                                                                           \
  {                                                                                                            \
    if (!value)                                                                                                \
    {                                                                                                          \
      if (sizeof("" #__VA_ARGS__) > 1)                                                                         \
      {                                                                                                        \
        ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__); \
      }                                                                                                        \
      else                                                                                                     \
      {                                                                                                        \
        ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__);               \
      }                                                                                                        \
      return std::unexpected(value.error());                                                                   \
    }                                                                                                          \
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
#define RETURN_ERROR_ON_UNEXPECTED(value, log_tag, ...)                                                        \
  do                                                                                                           \
  {                                                                                                            \
    if (!value)                                                                                                \
    {                                                                                                          \
      if (sizeof("" #__VA_ARGS__) > 1)                                                                         \
      {                                                                                                        \
        ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__); \
      }                                                                                                        \
      else                                                                                                     \
      {                                                                                                        \
        ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(value.error()), __FUNCTION__, __LINE__);               \
      }                                                                                                        \
      return value.error();                                                                                    \
    }                                                                                                          \
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
#define RETURN_UNEXPECTED_ON_ERROR(value, log_tag, ...)                                                  \
  do                                                                                                     \
  {                                                                                                      \
    esp_err_t err_rc_ = (value);                                                                         \
    if (unlikely(err_rc_ != ESP_OK))                                                                     \
    {                                                                                                    \
      if (sizeof("" #__VA_ARGS__) > 1)                                                                   \
      {                                                                                                  \
        ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__); \
      }                                                                                                  \
      else                                                                                               \
      {                                                                                                  \
        ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__);               \
      }                                                                                                  \
      return std::unexpected(err_rc_);                                                                   \
    }                                                                                                    \
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
#define RETURN_VOID_ON_ERROR(value, log_tag, ...)                                                        \
  do                                                                                                     \
  {                                                                                                      \
    esp_err_t err_rc_ = (value);                                                                         \
    if (unlikely(err_rc_ != ESP_OK))                                                                     \
    {                                                                                                    \
      if (sizeof("" #__VA_ARGS__) > 1)                                                                   \
      {                                                                                                  \
        ESP_LOGE(log_tag, __VA_ARGS__ ": %s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__); \
      }                                                                                                  \
      else                                                                                               \
      {                                                                                                  \
        ESP_LOGE(log_tag, "%s (%s:%d)", esp_err_to_name(err_rc_), __FUNCTION__, __LINE__);               \
      }                                                                                                  \
      return;                                                                                            \
    }                                                                                                    \
  } while (0)

/**
 * Macro to return an `std::unexpected` when a value is `false`.
 */
#define RETURN_UNEXPECTED_ON_FALSE(value, unexpected_value) \
  do                                                        \
  {                                                         \
    if (unlikely(!(value)))                                 \
    {                                                       \
      return std::unexpected(unexpected_value);             \
    }                                                       \
  } while (0)

} // namespace cjf

#endif /* F71A9988_B330_430E_A40E_9E585BBA5F2C */
