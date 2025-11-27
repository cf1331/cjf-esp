#ifndef DD1AC017_3428_492C_A514_64D64F435F6F
#define DD1AC017_3428_492C_A514_64D64F435F6F

#include "cjf/gpio.h"
#include <array>
#include <concepts>
#include <cstdint>
#include <esp_err.h>
#include <expected>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

namespace cjf::atomic_gpio
{
  // Simple concept: pin has a device() method
  template <typename T>
  concept has_device_method = requires(const T &t) {
    t.device();
  };

  // Helper to get device() return type
  template <typename T>
  using device_return_t = decltype(std::declval<const T &>().device());

  // Check if all device() return types are the same
  template <typename First, typename... Rest>
  constexpr bool same_device_return_types_v = (std::same_as<device_return_t<First>, device_return_t<Rest>> && ...);

  template <typename Device, gpio_like Pin>
  bool belong_to_device(const Device *device, std::initializer_list<std::reference_wrapper<Pin>> pins) noexcept
  {
    if (!device)
    {
      return false;
    }
    for (const Pin &pin : pins)
    {
      auto pin_device = pin.device().lock();
      if (!pin_device || pin_device.get() != device)
      {
        return false;
      }
    }
    return true;
  }

  template <cjf::gpio_like... Pins>
  decltype(std::get<0>(std::make_tuple(std::declval<Pins>().device()...)).lock()) common_device(Pins &...pins)
  {
    using device_type = decltype(std::get<0>(std::make_tuple(pins.device()...)).lock());

    if constexpr (sizeof...(pins) == 0 || !(has_device_method<Pins> && ...) || (sizeof...(pins) > 1 && !same_device_return_types_v<Pins...>))
    {
      return device_type{};
    }
    else
    {
      // Get the first device to compare against
      auto first_device = [&]()
      {
        auto devices = std::make_tuple(pins.device()...);
        return std::get<0>(devices).lock();
      }();

      if (!first_device)
      {
        return device_type{};
      }

      // Check if all devices are the same using fold expression
      bool all_same = ((pins.device().lock() == first_device) && ...);

      if (!all_same)
      {
        return device_type{};
      }

      // All pins belong to the same device, so return that device
      return first_device;
    }
  }

  // Variadic template function that accepts any gpio-like pins
  template <cjf::gpio_like... Pins>
  std::expected<uint32_t, esp_err_t> try_read_pins(Pins &...pins) noexcept
  {
    auto device = common_device(pins...);
    return device
        ? device->read_pins({pins...})
        : ESP_ERR_NOT_SUPPORTED;
  }

  // Variadic template function that accepts any gpio-like pins
  template <cjf::gpio_like... Pins>
  esp_err_t try_write_pins(uint32_t value, Pins &...pins) noexcept
  {
    auto device = common_device(pins...);
    return device
        ? device->write_pins(value, {pins...})
        : ESP_ERR_NOT_SUPPORTED;
  }
} // namespace cjf::atomic_gpio

#endif /* DD1AC017_3428_492C_A514_64D64F435F6F */
