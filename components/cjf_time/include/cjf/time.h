#ifndef A5FA5DCC_A0EE_45B6_8999_72D9BF3DF705
#define A5FA5DCC_A0EE_45B6_8999_72D9BF3DF705

/**
 * @file time.h
 * @brief Time synchronization and management utilities for ESP-IDF
 *
 * This file provides C++ utilities for time synchronization, timezone management,
 * and time format conversion. It includes concepts for time sources and sinks,
 * template functions for synchronizing time between different devices/sources,
 * and utilities for working with timezones and ISO 8601 format.
 */

#include "cjf/timezones.h"
#include <cjf/error_handling.h>
#include <cjf/event.h>
#include <cjf/nvs.h>
#include <ctime>
#include <expected>
#include <string>
#include <sys/time.h>

namespace cjf
{
  /**
   * @brief Concept for objects that can receive time updates
   *
   * A time_sink is any type that provides a `set_time()` method accepting
   * a `timeval` structure and returning an `esp_err_t` error code.
   *
   * @tparam T Type to check for time_sink compliance
   */
  template <typename T>
  concept time_sink = requires(T &sink, const timeval &tv) {
    { sink.set_time(tv) } -> std::convertible_to<esp_err_t>;
  };

  /**
   * @brief Concept for objects that can provide time information
   *
   * A time_source is any type that provides a `get_time()` method returning
   * an `std::expected<timeval, esp_err_t>` containing the current time or an error.
   *
   * @tparam T Type to check for time_source compliance
   */
  template <typename T>
  concept time_source = requires(T &source) {
    { source.get_time() } -> std::convertible_to<std::expected<timeval, esp_err_t>>;
  };

  /**
   * @brief Load timezone configuration from NVS storage
   *
   * Attempts to load the timezone configuration from NVS storage. If no timezone
   * is found in NVS, the provided default timezone will be used.
   *
   * @param nvs Expected containing NVS handle for reading timezone data
   * @param default_tz Default POSIX timezone string to use if none found in NVS
   * @return `ESP_OK` on success, error code on failure
   */
  esp_err_t load_timezone(const std::expected<nvs, esp_err_t> &nvs, const char *default_tz);

  /**
   * @brief Set the system timezone
   *
   * Sets the system timezone using a POSIX timezone string (e.g., "UTC-0",
   * "EST5EDT,M3.2.0,M11.1.0").
   *
   * @param posix_tz POSIX timezone string specifying the timezone
   * @return `ESP_OK` on success, error code on failure
   */
  esp_err_t set_timezone(const char *posix_tz);

  /**
   * @brief Convert timeval to ISO 8601 string format
   *
   * Converts a timeval structure to ISO 8601 formatted string and stores it
   * in the provided buffer.
   *
   * @param tv The timeval structure to convert
   * @param dst Destination buffer to store the ISO 8601 string
   * @param dst_size Size of the destination buffer
   * @return Pointer to the destination buffer (same as `dst` parameter)
   */
  char *to_iso8061(const timeval &tv, char *dst, size_t dst_size);

  /**
   * @brief Convert timeval to ISO 8601 string format
   *
   * Converts a timeval structure to ISO 8601 formatted string and returns it
   * as a std::string.
   *
   * @param tv The timeval structure to convert
   * @return ISO 8601 formatted string representation of the time
   */
  std::string to_iso8061(const timeval &tv);

  /**
   * @brief Synchronize system time from a time source
   *
   * Gets the current time from the specified time source and sets the system time
   * accordingly. Logs the synchronization result.
   *
   * @tparam T Type that satisfies the time_source concept
   * @param src The time source to read time from
   * @param src_name Human-readable name for the source (used in logging)
   * @return `ESP_OK` on successful synchronization, error code on failure
   */
  template <time_source T>
  esp_err_t sync_time_from(const T &src, const char *src_name = "source") noexcept
  {
    static constexpr const char *TAG = "cjf:time";
    auto tv = src.get_time();
    RETURN_ERROR_ON_UNEXPECTED(tv, TAG);
    RETURN_ON_ERROR(settimeofday(&*tv, nullptr), TAG);
    ESP_LOGI(TAG, "Time synced from %s: %s", src_name, to_iso8061(*tv).c_str());
    return ESP_OK;
  }

  /**
   * @brief Synchronize system time from an expected time source
   *
   * Wrapper for sync_time_from that handles std::expected<T, esp_err_t> sources.
   * If the expected contains an error, that error is returned without attempting
   * synchronization.
   *
   * @tparam T Type that satisfies the time_source concept
   * @param src Expected containing the time source or an error
   * @param src_name Human-readable name for the source (used in logging)
   * @return `ESP_OK` on successful synchronization, error code on failure
   */
  template <time_source T>
  esp_err_t sync_time_from(const std::expected<T, esp_err_t> &src, const char *src_name = "source") noexcept
  {
    RETURN_ERROR_ON_UNEXPECTED(src, "cjf:time");
    return sync_time_from(*src, src_name);
  }

  /**
   * @brief Context structure for event-driven time synchronization
   *
   * Holds the destination time sink and its name for use in event handlers
   * that synchronize time when specific events occur.
   *
   * @tparam T Type that satisfies the time_sink concept
   */
  template <time_sink T>
  struct sync_time_on_ctx
  {
    /// Pointer to the time sink destination
    const T *dst;
    /// Human-readable name for the destination (used in logging)
    const char *dst_name;
  };

  /**
   * @brief Set up automatic time synchronization on events
   *
   * Creates an event handler that automatically synchronizes the system time
   * to the specified time sink whenever the given event occurs. The event handler
   * manages its own memory and will clean up properly when destroyed.
   *
   * @tparam T Type that satisfies the time_sink concept
   * @param event_base The event base to listen for (e.g., WIFI_EVENT, IP_EVENT)
   * @param event_id The specific event ID to trigger synchronization
   * @param dst The time sink to synchronize to when the event occurs
   * @param dst_name Human-readable name for the destination (used in logging)
   * @return Expected containing event_handler on success, or esp_err_t on failure
   */
  template <time_sink T>
  std::expected<event_handler, esp_err_t> sync_time_on(esp_event_base_t event_base, int32_t event_id, const T &dst, const char *dst_name = "sink") noexcept
  {
    static constexpr const char *TAG = "cjf:time";
    auto event_handler = [](void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
      auto ctx = static_cast<sync_time_on_ctx<T> *>(arg);
      LOG_IF_ERROR(sync_time_to(*ctx->dst, ctx->dst_name), TAG, "Failed to sync time on event");
    };
    // clang-format off
    return event_handler::create_with_managed_arg(
        event_base, event_id,
        event_handler,
        new sync_time_on_ctx<T>{&dst, dst_name},
        [](void *ptr) { delete static_cast<sync_time_on_ctx<T> *>(ptr); });
    // clang-format on
  }

  /**
   * @brief Synchronize current system time to a time sink
   *
   * Gets the current system time and writes it to the specified time sink.
   * Logs the synchronization result.
   *
   * @tparam T Type that satisfies the time_sink concept
   * @param dst The time sink to write the current time to
   * @param dst_name Human-readable name for the destination (used in logging)
   * @return `ESP_OK` on successful synchronization, error code on failure
   */
  template <time_sink T>
  esp_err_t sync_time_to(const T &dst, const char *dst_name = "sink") noexcept
  {
    static constexpr const char *TAG = "cjf:time";
    timeval tv;
    RETURN_ON_ERROR(gettimeofday(&tv, nullptr), TAG);
    RETURN_ON_ERROR(dst.set_time(tv), TAG);
    ESP_LOGI(TAG, "Time synced to %s: %s", dst_name, to_iso8061(tv).c_str());
    return ESP_OK;
  }

  /**
   * @brief Synchronize current system time to an expected time sink
   *
   * Wrapper for sync_time_to that handles std::expected<T, esp_err_t> destinations.
   * If the expected contains an error, that error is returned without attempting
   * synchronization.
   *
   * @tparam T Type that satisfies the time_sink concept
   * @param dst Expected containing the time sink or an error
   * @param dst_name Human-readable name for the destination (used in logging)
   * @return `ESP_OK` on successful synchronization, error code on failure
   */
  template <time_sink T>
  esp_err_t sync_time_to(const std::expected<T, esp_err_t> &dst, const char *dst_name = "sink") noexcept
  {
    RETURN_ON_UNEXPECTED(dst, "cjf:time");
    return sync_time_to(*dst, dst_name);
  }
} // namespace cjf

#endif /* A5FA5DCC_A0EE_45B6_8999_72D9BF3DF705 */
