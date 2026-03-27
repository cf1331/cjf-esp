#ifndef A08EFEB4_9F01_496E_9A1F_D298CB3FBFE2
#define A08EFEB4_9F01_496E_9A1F_D298CB3FBFE2

#include <cjf/error_handling.h>
#include <cjf/gpio.h>
#include <concepts>
#include <esp_err.h>
#include <expected>
#include <freertos/FreeRTOS.h>
#include <memory>

namespace cjf
{
  template <gpio_like ResetPin>
  class has_gpio_reset
  {
  public:
    struct config_type
    {
      std::expected<ResetPin, esp_err_t> reset_pin;
      bool reset_level = false;
      const char *log_tag = nullptr;
      TickType_t reset_duration = 0;   // Duration of reset signal (in ticks)
      TickType_t post_reset_delay = 0; // Delay after reset to allow device start-up (in ticks)
    };

    static std::expected<has_gpio_reset, esp_err_t> init(config_type &config) noexcept
    {
      RETURN_ON_UNEXPECTED(config.reset_pin, config.log_tag);
      // Default to de-asserting the reset line
      ESP_LOGI(config.log_tag, "Setting reset to de-asserted level: %d", !config.reset_level);
      RETURN_UNEXPECTED_ON_ERROR(config.reset_pin->set_level(!config.reset_level), config.log_tag);
      return has_gpio_reset(config);
    }

    inline esp_err_t reset() noexcept
    {
      if (log_tag_)
      {
        ESP_LOGI(log_tag_, "Resetting");
      }
      RETURN_ON_ERROR(reset_pin_.set_level(reset_level_), TAG);
      if (reset_duration_ > 0)
      {
        vTaskDelay(reset_duration_);
      }
      RETURN_ON_ERROR(reset_pin_.set_level(!reset_level_), TAG);
      return ESP_OK;
    }

  protected:
    constexpr has_gpio_reset(config_type &config) noexcept
        : reset_pin_(std::move(*config.reset_pin)),
          reset_level_(config.reset_level),
          log_tag_(config.log_tag),
          reset_duration_(config.reset_duration),
          post_reset_delay_(config.post_reset_delay) {}

  private:
    constexpr const static char *TAG = "cjf:power";

    ResetPin reset_pin_;
    bool reset_level_;
    const char *log_tag_;
    TickType_t reset_duration_;
    TickType_t post_reset_delay_;
  };
} // namespace cjf

#endif /* A08EFEB4_9F01_496E_9A1F_D298CB3FBFE2 */
