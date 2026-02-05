#ifndef E299AB45_CDB3_4F6F_ACA3_32FDE0F26E11
#define E299AB45_CDB3_4F6F_ACA3_32FDE0F26E11

#include <cjf/error_handling.h>
#include <cjf/gpio.h>
#include <concepts>
#include <esp_err.h>
#include <expected>
#include <memory>

namespace cjf
{
  template <gpio_like PowerGoodPin>
  class has_power_good_signal
  {
  public:
    struct config_type
    {
      std::expected<PowerGoodPin, esp_err_t> power_good_pin;
      bool good_level = true;
    };

    static std::expected<has_power_good_signal<PowerGoodPin>, esp_err_t> &init(
        std::expected<has_power_good_signal<PowerGoodPin>, esp_err_t> &inst,
        config_type &config) noexcept
    {
      if (!config.power_good_pin)
      {
        inst = std::unexpected(config.power_good_pin.error());
      }
      else
      {
        inst.emplace(config);
      }
      return inst;
    }

    std::expected<bool, esp_err_t> is_power_good() noexcept
    {
      auto level = power_good_pin_.get_level();
      if (!level)
      {
        return std::unexpected(level.error());
      }
      return *level == good_level_;
    }

    constexpr has_power_good_signal(config_type &config) noexcept
        : power_good_pin_(std::move(*config.power_good_pin)),
          good_level_(config.good_level) {}

  private:
    PowerGoodPin power_good_pin_;
    bool good_level_;
  };

} // namespace cjf

#endif /* E299AB45_CDB3_4F6F_ACA3_32FDE0F26E11 */
