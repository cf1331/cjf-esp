#ifndef AA8DE5A3_E3BD_4274_A2D5_738801B84CC4
#define AA8DE5A3_E3BD_4274_A2D5_738801B84CC4

/**
 * @file countdown.h
 * @brief Countdown timer functionality for ESP-IDF
 *
 * This file provides countdown timer functions that can display countdown messages
 * to the console with configurable start and completion messages.
 */

#include <cstdint>
#include <esp_log.h>

namespace cjf
{
  /**
   * @brief Configuration structure for countdown operations
   *
   * Contains all parameters needed to configure a countdown timer, including
   * optional start and completion messages, duration, and logging level.
   */
  struct countdown_config_type
  {
    /// Optional message displayed when countdown starts (nullptr to skip)
    const char *start_message = nullptr;

    /// Duration of countdown in seconds
    uint32_t seconds;

    /// Optional message displayed when countdown completes (nullptr to skip)
    const char *go_message = nullptr;

    /// Log level for countdown messages
    esp_log_level_t log_level = ESP_LOG_WARN;
  };

  /**
   * @brief Perform a simple countdown for the specified duration
   *
   * Displays a countdown from the specified number of seconds down to 0,
   * with each second displayed on the console.
   *
   * @param seconds Duration of countdown in seconds
   */
  void countdown(uint32_t seconds) noexcept;

  /**
   * @brief Perform a configurable countdown with optional messages
   *
   * Displays a countdown according to the provided configuration, including
   * optional start and completion messages at the specified log level.
   *
   * @param config Configuration structure containing countdown parameters
   */
  void countdown(const countdown_config_type &config) noexcept;
}

#endif /* AA8DE5A3_E3BD_4274_A2D5_738801B84CC4 */
