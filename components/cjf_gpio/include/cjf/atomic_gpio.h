#ifndef DD1AC017_3428_492C_A514_64D64F435F6F
#define DD1AC017_3428_492C_A514_64D64F435F6F

#include "cjf/gpio.h"
#include <concepts>
#include <cstdint>
#include <esp_err.h>
#include <expected>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace cjf::atomic_gpio
{
  // Helper to get device() return type
  template <typename T>
  using device_return_t = decltype(std::declval<const T &>().device());

  // Detect if device() returns a reference type
  template <typename T>
  concept returns_reference = std::is_lvalue_reference_v<device_return_t<T>>;

  // Detect if device() returns a pointer type
  template <typename T>
  concept returns_pointer = std::is_pointer_v<device_return_t<T>>;

  // Check if all device() return types are the same
  template <typename First, typename... Rest>
  constexpr bool same_device_return_types_v = (std::same_as<device_return_t<First>, device_return_t<Rest>> && ...);

  // For types where device() returns a reference (e.g., TCAL6416&)
  template <cjf::gpio_like... Pins>
    requires(sizeof...(Pins) > 0 && (returns_reference<Pins> && ...))
  auto common_device(Pins &...pins) -> std::remove_reference_t<device_return_t<std::tuple_element_t<0, std::tuple<Pins...>>>> *
  {
    using device_type = std::remove_reference_t<device_return_t<std::tuple_element_t<0, std::tuple<Pins...>>>>;

    if constexpr (sizeof...(pins) > 1 && !same_device_return_types_v<Pins...>)
    {
      return nullptr;
    }
    else
    {
      // Get the first device to compare against
      device_type *first_device = &std::get<0>(std::make_tuple(std::ref(pins.device())...)).get();

      // Check if all devices are the same using fold expression
      bool all_same = ((&pins.device() == first_device) && ...);

      if (!all_same)
      {
        return nullptr;
      }

      // All pins belong to the same device, so return that device
      return first_device;
    }
  }

  // For types where device() returns a pointer (e.g., void* or Device*)
  template <cjf::gpio_like... Pins>
    requires(sizeof...(Pins) > 0 && (returns_pointer<Pins> && ...))
  auto common_device(Pins &...pins) -> std::remove_pointer_t<device_return_t<std::tuple_element_t<0, std::tuple<Pins...>>>> *
  {
    using device_ptr_type = device_return_t<std::tuple_element_t<0, std::tuple<Pins...>>>;
    using device_type = std::remove_pointer_t<device_ptr_type>;

    if constexpr (sizeof...(pins) > 1 && !same_device_return_types_v<Pins...>)
    {
      return nullptr;
    }
    else
    {
      // Get the first device to compare against
      device_type *first_device = std::get<0>(std::make_tuple(pins.device()...));

      // nullptr means no device, can't do atomic operations
      if (!first_device)
      {
        return nullptr;
      }

      // Check if all devices are the same using fold expression
      bool all_same = ((pins.device() == first_device) && ...);

      if (!all_same)
      {
        return nullptr;
      }

      // All pins belong to the same device, so return that device
      return first_device;
    }
  }

  // Variadic template function that accepts gpio-like pins returning references
  template <cjf::gpio_like... Pins>
    requires(sizeof...(Pins) > 0 && (returns_reference<Pins> && ...))
  std::expected<uint32_t, esp_err_t> try_read_pins(Pins &...pins) noexcept
  {
    auto device = common_device(pins...);
    return device
               ? device->read_pins({pins...})
               : std::unexpected(ESP_ERR_NOT_SUPPORTED);
  }

  // Variadic template function that accepts gpio-like pins returning pointers
  template <cjf::gpio_like... Pins>
    requires(sizeof...(Pins) > 0 && (returns_pointer<Pins> && ...))
  std::expected<uint32_t, esp_err_t> try_read_pins(Pins &...pins) noexcept
  {
    auto device = common_device(pins...);
    return device
               ? device->read_pins({pins...})
               : std::unexpected(ESP_ERR_NOT_SUPPORTED);
  }

  // Variadic template function that accepts gpio-like pins returning references
  template <cjf::gpio_like... Pins>
    requires(sizeof...(Pins) > 0 && (returns_reference<Pins> && ...))
  esp_err_t try_write_pins(uint32_t value, Pins &...pins) noexcept
  {
    auto device = common_device(pins...);
    return device
               ? device->write_pins(value, {pins...})
               : ESP_ERR_NOT_SUPPORTED;
  }

  // Variadic template function that accepts gpio-like pins returning pointers
  template <cjf::gpio_like... Pins>
    requires(sizeof...(Pins) > 0 && (returns_pointer<Pins> && ...))
  esp_err_t try_write_pins(uint32_t value, Pins &...pins) noexcept
  {
    auto device = common_device(pins...);
    return device
               ? device->write_pins(value, {pins...})
               : ESP_ERR_NOT_SUPPORTED;
  }
} // namespace cjf::atomic_gpio

#endif /* DD1AC017_3428_492C_A514_64D64F435F6F */
