#ifndef B68F88A8_2AFB_41E4_9B20_0277C03F71C2
#define B68F88A8_2AFB_41E4_9B20_0277C03F71C2

#include <cjf/error_handling.h>
#include <cjf/gpio.h>
#include <cjf/shared_guard.h>
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

    /**
     * @brief Deactivator functor for power-off on last guard release
     */
    struct power_off_deactivator
    {
      has_gpio_enabled_power *resource;

      void operator()() const noexcept
      {
        LOG_IF_ERROR(resource->do_power_off(), TAG, "Failed to power off");
      }
    };

    using power_guard = cjf::shared_guard<power_off_deactivator>;

    static std::expected<has_gpio_enabled_power, esp_err_t>& init(
        std::expected<has_gpio_enabled_power, esp_err_t> &inst,
        config_type &config) noexcept
    {
      if (!config.enable_pin)
      {
        inst = std::unexpected(config.enable_pin.error());
      }
      else
      {
        inst.emplace(config);
      }
      return inst;
    }

    /**
     * @brief Acquire a guard for this power rail
     *
     * Powers on the rail if this is the first guard. Returns a RAII guard that
     * will power off the rail when the last guard is destroyed.
     *
     * @return Guard on success, or error if power-on fails
     */
    [[nodiscard]] std::expected<power_guard, esp_err_t> power_on() noexcept
    {
      // Check if we need to activate the hardware
      if (power_guard::needs_activation(guard_ctrl_))
      {
        RETURN_UNEXPECTED_ON_ERROR(do_power_on(), TAG, "Failed to power on");
      }

      // Return guard (increments refcount)
      return power_guard(guard_ctrl_);
    }

    constexpr has_gpio_enabled_power(config_type &config) noexcept
        : enable_pin_(std::move(*config.enable_pin)),
          enabled_level_(config.enabled_level),
          log_tag_(config.log_tag),
          power_off_delay_(config.power_off_delay),
          power_on_delay_(config.power_on_delay),
          guard_ctrl_(power_off_deactivator{this}) {}

  private:
    /**
     * @brief Internal power-on implementation
     */
    esp_err_t do_power_on() noexcept
    {
      RETURN_ON_ERROR(enable_pin_.set_level(enabled_level_), TAG);
      if (power_on_delay_ > 0)
      {
        vTaskDelay(power_on_delay_);
      }
      if (log_tag_)
      {
        ESP_LOGI(log_tag_, "Powered on");
      }
      return ESP_OK;
    }

    /**
     * @brief Internal power-off implementation (called by deactivator)
     */
    esp_err_t do_power_off() noexcept
    {
      RETURN_ON_ERROR(enable_pin_.set_level(!enabled_level_), TAG);
      if (power_off_delay_ > 0)
      {
        vTaskDelay(power_off_delay_);
      }
      if (log_tag_)
      {
        ESP_LOGI(log_tag_, "Powered off");
      }
      return ESP_OK;
    }

    EnablePin enable_pin_;
    bool enabled_level_;
    const char *log_tag_;
    TickType_t power_off_delay_;
    TickType_t power_on_delay_;
    power_guard::control_block guard_ctrl_;
  };

} // namespace cjf

#endif /* B68F88A8_2AFB_41E4_9B20_0277C03F71C2 */
