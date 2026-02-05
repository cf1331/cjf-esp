#ifndef C140C92E_A9F9_407A_8346_CE1528D90803
#define C140C92E_A9F9_407A_8346_CE1528D90803

#include <cjf/scope_guard.h>
#include <concepts>
#include <driver/gpio.h>
#include <esp_err.h>
#include <expected>
#include <experimental/scope>
#include <memory>
#include <type_traits>

namespace cjf
{
  template <typename T>
  concept gpio_like = requires(T &pin) {
    { pin.get_level() } -> std::same_as<std::expected<bool, esp_err_t>>;
    { pin.set_level(bool{}) } -> std::same_as<esp_err_t>;
    { pin.reconfigure(gpio_mode_t{}, gpio_pull_mode_t{}, gpio_int_type_t{}) } -> std::same_as<esp_err_t>;
  };

  template <typename T>
  concept gpio_pin_with_timeout = gpio_like<T> && requires(T &pin) {
    { pin.get_level(int32_t{}) } -> std::same_as<std::expected<bool, esp_err_t>>;
    { pin.set_level(bool{}, int32_t{}) } -> std::same_as<esp_err_t>;
    { pin.reconfigure(gpio_mode_t{}, gpio_pull_mode_t{}, gpio_int_type_t{}, int32_t{}) } -> std::same_as<esp_err_t>;
  };

  template <typename T>
  constexpr bool has_timeout_support_v = gpio_pin_with_timeout<T>;

  class gpio
  {
  public:
    static std::expected<gpio, esp_err_t> init(gpio_config_t config);
    static std::expected<gpio, esp_err_t> init(
        gpio_num_t gpio_num,
        gpio_mode_t mode,
        gpio_pull_mode_t pull = GPIO_FLOATING,
        gpio_int_type_t intr_type = GPIO_INTR_DISABLE);

    // Implicit conversion to gpio_num_t to allow use with esp-idf gpio API
    constexpr operator gpio_num_t() const noexcept { return gpio_num_.get(); }

    // Core GPIO operations - inline for zero overhead
    inline std::expected<bool, esp_err_t> get_level() const noexcept
    {
      return gpio_get_level(gpio_num_.get()) != 0;
    }

    inline esp_err_t set_level(bool level) const noexcept
    {
      return gpio_set_level(gpio_num_.get(), level ? 1 : 0);
    }

    inline esp_err_t reconfigure(gpio_mode_t mode,
                                 gpio_pull_mode_t pull = GPIO_FLOATING,
                                 gpio_int_type_t intr_type = GPIO_INTR_DISABLE) const noexcept
    {
      gpio_config_t config = make_gpio_config(gpio_num_.get(), mode, pull, intr_type);
      return gpio_config(&config);
    }

    esp_err_t reconfigure(gpio_config_t config) const;

    // Additional ESP32-specific operations
    esp_err_t hold_disable() const;
    esp_err_t hold_enable() const;
    esp_err_t intr_disable() const;
    esp_err_t intr_enable() const;
    esp_err_t wakeup_disable() const;
    esp_err_t wakeup_enable(gpio_int_type_t intr_type) const;

    gpio_drive_cap_t get_drive_capability() const;
    esp_err_t set_drive_capability(gpio_drive_cap_t strength) const;
    esp_err_t set_pull_mode(gpio_pull_mode_t pull) const;

    constexpr void* device() const noexcept { return nullptr; }

    // Device interface
    constexpr bool supports_atomic_writes() const noexcept { return true; }
    constexpr gpio_num_t pin() const noexcept { return gpio_num_.get(); }

  private:
    struct deleter
    {
      void operator()(gpio_num_t gpio_num) const noexcept;
    };

    std::experimental::unique_resource<gpio_num_t, deleter> gpio_num_;

    explicit gpio(gpio_num_t gpio_num) noexcept;

    static constexpr gpio_config_t make_gpio_config(
        gpio_num_t gpio_num,
        gpio_mode_t mode,
        gpio_pull_mode_t pull,
        gpio_int_type_t intr_type) noexcept
    {
      gpio_pullup_t pull_up_en = (pull == GPIO_PULLUP_ONLY || pull == GPIO_PULLUP_PULLDOWN)
          ? GPIO_PULLUP_ENABLE
          : GPIO_PULLUP_DISABLE;
      gpio_pulldown_t pull_down_en = (pull == GPIO_PULLDOWN_ONLY || pull == GPIO_PULLUP_PULLDOWN)
          ? GPIO_PULLDOWN_ENABLE
          : GPIO_PULLDOWN_DISABLE;
      return {
          .pin_bit_mask = 1ULL << gpio_num,
          .mode = mode,
          .pull_up_en = pull_up_en,
          .pull_down_en = pull_down_en,
          .intr_type = intr_type,
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
          .hys_ctrl_mode = GPIO_HYS_CTRL_EFUSE,
#endif
      };
    }
  };

  class gpio_isr_service
  {
  public:
    // Factory: installs the ISR service and returns an owning instance on success
    static std::expected<gpio_isr_service, esp_err_t> install();

    esp_err_t add_handler(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args = nullptr);
    esp_err_t remove_handler(gpio_num_t gpio_num);

  private:
    struct deleter
    {
      void operator()() const noexcept;
    };

    cjf::scope_guard<gpio_isr_service::deleter> scope_guard_;
  };

} // namespace cjf

#endif // C140C92E_A9F9_407A_8346_CE1528D90803
