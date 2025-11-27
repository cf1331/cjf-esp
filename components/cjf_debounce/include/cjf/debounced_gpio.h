#ifndef BB49337D_6CC7_46F4_B18D_D1AD24715F79
#define BB49337D_6CC7_46F4_B18D_D1AD24715F79

#include <cjf/gpio.h>

namespace cjf
{

  template <gpio_like Gpio>
  class debounced_gpio
  {
  public:
    struct config_type
    {
      Gpio gpio;
      uint32_t stable_time_ms = 5;
    };

    static std::expected<debounced_gpio, esp_err_t> init(config_type& config)
    {
      if (debounce_time_ms < 0) return std::unexpected(ESP_ERR_INVALID_ARG);
      return debounced_gpio(std::move(gpio), debounce_time_ms);
    }

    std::expected<bool, esp_err_t> get_level() noexcept
    {

    }

    esp_err_t reconfigure(gpio_mode_t mode, gpio_pull_mode_t pull = GPIO_FLOATING, gpio_int_type_t intr_type = GPIO_INTR_DISABLE) noexcept
    {
      return gpio_.reconfigure(mode, pull, intr_type);
    }

    esp_err_t set_level(bool level) noexcept;

  private:
    Gpio gpio_;
    uint32_t stable_time_ms_;

    mutable bool last_state_ = false;
  };

}

#endif /* BB49337D_6CC7_46F4_B18D_D1AD24715F79 */
