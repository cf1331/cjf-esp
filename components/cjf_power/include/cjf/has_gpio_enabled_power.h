#ifndef B68F88A8_2AFB_41E4_9B20_0277C03F71C2
#define B68F88A8_2AFB_41E4_9B20_0277C03F71C2

#include <cjf/error_handling.h>
#include <cjf/gpio.h>
#include <concepts>
#include <esp_err.h>
#include <expected>
#include <freertos/FreeRTOS.h>
#include <memory>

namespace cjf
{
  static const char *TAG = "cjf:power";

  template <gpio_like EnablePin>
  class has_gpio_enabled_power
  {
  public:
    struct config_type
    {
      std::expected<EnablePin, esp_err_t> enable_pin;
      bool enabled_level = true;
      const char *log_tag = nullptr;
      TickType_t power_on_delay = 0;  // Delay after powering on (in ticks)
      TickType_t power_off_delay = 0; // Delay after powering off (in ticks)
    };

    static std::expected<has_gpio_enabled_power, esp_err_t> init(config_type &config) noexcept
    {
      if (!config.enable_pin)
      {
        return std::unexpected(config.enable_pin.error());
      }
      return has_gpio_enabled_power(config);
    }

    inline esp_err_t power_off() noexcept
    {
      RETURN_ON_ERROR(enable_pin_.set_level(!enabled_level_), TAG);
      if (power_on_delay_ > 0)
      {
        vTaskDelay(power_on_delay_);
      }
      if (log_tag_)
      {
        ESP_LOGI(log_tag_, "Powered off");
      }
      return ESP_OK;
    }

    inline esp_err_t power_on() noexcept
    {
      RETURN_ON_ERROR(enable_pin_.set_level(enabled_level_), TAG);
      if (power_off_delay_ > 0)
      {
        vTaskDelay(power_off_delay_);
      }
      if (log_tag_)
      {
        ESP_LOGI(log_tag_, "Powered on");
      }
      return ESP_OK;
    }

  protected:
    constexpr has_gpio_enabled_power(config_type &config) noexcept
        : enable_pin_(std::move(*config.enable_pin)),
          enabled_level_(config.enabled_level),
          log_tag_(config.log_tag),
          power_off_delay_(config.power_off_delay),
          power_on_delay_(config.power_on_delay) {}

  private:
    EnablePin enable_pin_;
    bool enabled_level_;
    const char *log_tag_;
    TickType_t power_off_delay_;
    TickType_t power_on_delay_;
  };

} // namespace cjf

#endif /* B68F88A8_2AFB_41E4_9B20_0277C03F71C2 */
