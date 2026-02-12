#include "cjf/gpio.h"
#include <cjf/error_handling.h>
#include <stdio.h>

static const char *CJF_GPIO = "cjf::gpio";

namespace cjf
{
  std::expected<gpio, esp_err_t> gpio::init(gpio_config_t config)
  {
    // Only support single-pin config
    if (__builtin_popcountll(config.pin_bit_mask) != 1)
    {
      return std::unexpected(ESP_ERR_INVALID_ARG);
    }
    RETURN_UNEXPECTED_ON_ERROR(gpio_config(&config), CJF_GPIO);
    gpio_num_t gpio_num = static_cast<gpio_num_t>(__builtin_ctzll(config.pin_bit_mask));
    return gpio(gpio_num);
  }

  std::expected<gpio, esp_err_t> gpio::init(
      gpio_num_t gpio_num,
      gpio_mode_t mode,
      gpio_pull_mode_t pull,
      gpio_int_type_t intr_type)
  {
    gpio_config_t config = make_gpio_config(gpio_num, mode, pull, intr_type);
    RETURN_UNEXPECTED_ON_ERROR(gpio_config(&config), CJF_GPIO);
    return gpio(gpio_num);
  }

  gpio::gpio(gpio_num_t gpio_num) noexcept : gpio_num_{gpio_num, deleter{}} {}

  esp_err_t gpio::reconfigure(gpio_config_t config) const
  {
    return gpio_config(&config);
  }

  esp_err_t gpio::hold_disable() const
  {
    return gpio_hold_dis(gpio_num_.get());
  }

  esp_err_t gpio::hold_enable() const
  {
    return gpio_hold_en(gpio_num_.get());
  }

  esp_err_t gpio::intr_disable() const
  {
    return gpio_intr_disable(gpio_num_.get());
  }

  esp_err_t gpio::intr_enable() const
  {
    return gpio_intr_enable(gpio_num_.get());
  }

  esp_err_t gpio::wakeup_disable() const
  {
    return gpio_wakeup_disable(gpio_num_.get());
  }

  esp_err_t gpio::wakeup_enable(gpio_int_type_t intr_type) const
  {
    return gpio_wakeup_enable(gpio_num_.get(), intr_type);
  }

  gpio_drive_cap_t gpio::get_drive_capability() const
  {
    gpio_drive_cap_t strength;
    gpio_get_drive_capability(gpio_num_.get(), &strength);
    return strength;
  }

  esp_err_t gpio::set_drive_capability(gpio_drive_cap_t strength) const
  {
    return gpio_set_drive_capability(gpio_num_.get(), strength);
  }

  esp_err_t gpio::set_pull_mode(gpio_pull_mode_t pull) const
  {
    return gpio_set_pull_mode(gpio_num_.get(), pull);
  }

  void gpio::deleter::operator()(gpio_num_t gpio_num) const noexcept
  {
    LOG_IF_ERROR(gpio_reset_pin(static_cast<gpio_num_t>(gpio_num)), CJF_GPIO);
  }

  std::expected<gpio_isr_service, esp_err_t> gpio_isr_service::init() noexcept
  {
    RETURN_UNEXPECTED_ON_ERROR(gpio_install_isr_service(0), CJF_GPIO);
    ESP_LOGI(CJF_GPIO, "GPIO ISR service installed");
    return gpio_isr_service();
  }

  void gpio_isr_service::handler_deleter::operator()() const noexcept
  {
    LOG_IF_ERROR(gpio_isr_handler_remove(gpio_num), CJF_GPIO);
    ESP_LOGI(CJF_GPIO, "GPIO ISR handler removed for GPIO %d", gpio_num);
  }

  std::expected<gpio_isr_service::handler_type, esp_err_t> gpio_isr_service::add_handler(
      gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args)
  {
    RETURN_UNEXPECTED_ON_ERROR(gpio_isr_handler_add(gpio_num, isr_handler, args), CJF_GPIO);
    ESP_LOGI(CJF_GPIO, "GPIO ISR handler added for GPIO %d", gpio_num);
    return handler_type({gpio_num});
  }

  void gpio_isr_service::deleter::operator()() const noexcept
  {
    gpio_uninstall_isr_service();
    ESP_LOGI(CJF_GPIO, "GPIO ISR service uninstalled");
  }

} // namespace cjf
