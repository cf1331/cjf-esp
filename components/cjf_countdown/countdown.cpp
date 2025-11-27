#include "cjf/countdown.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

namespace cjf
{
  static const char* TAG = "cjf:countdown";

  void countdown(uint32_t seconds) noexcept
  {
    countdown({.seconds = seconds});
  }

  void countdown(const countdown_config_type& config) noexcept
  {
    TickType_t last_time = xTaskGetTickCount();
    if (config.start_message)
    {
      ESP_LOG_LEVEL(config.log_level, TAG, "%s", config.start_message);
    }
    for (int i = config.seconds; i > 0; i--)
    {
      ESP_LOG_LEVEL(config.log_level, TAG, "%d...", i);
      xTaskDelayUntil(&last_time, pdMS_TO_TICKS(1000));
    }
    if (config.go_message)
    {
      ESP_LOG_LEVEL(config.log_level, TAG, "%s", config.go_message);
    }
  }

}