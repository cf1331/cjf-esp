#ifndef E5346742_E5F1_48E3_9463_6B2868629BC6
#define E5346742_E5F1_48E3_9463_6B2868629BC6

#include <cjf/string.h>
#include <cstdint>
#include <expected>
#include <stdexcept>
#include <string>
#include <variant>

namespace cjf
{
  template <class>
  inline constexpr bool always_false_v = false;

  enum param_error
  {
    ok = 0,
    no_value,
    read_only,
    invalid_cast,
    out_of_range,
  };

  using param_value = std::variant<
      int8_t,
      uint8_t,
      int16_t,
      uint16_t,
      int32_t,
      uint32_t,
      int64_t,
      uint64_t,
      bool,
      float,
      double,
      std::string>;

  inline constexpr auto param_null = std::unexpected(param_error::no_value);

  class param
  {
  public:
    using value_changed_func = void (*)(param &p, void *ctx);
    virtual std::expected<param_value, param_error> get() const noexcept = 0;
    virtual param_error set(const std::expected<param_value, param_error> &value) = 0;
    virtual void watch(value_changed_func callback, void *ctx = nullptr) = 0;
    virtual void unwatch(value_changed_func callback) = 0;

    template <typename T>
    const std::expected<T, param_error> get_as() const;

    constexpr bool has_value() const noexcept;
  };

  template <typename T>
  constexpr std::expected<T, param_error> param_cast(const param_value &value)
  {
    return std::visit(
        [](auto &&v) -> std::expected<T, param_error>
        {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, V>)
            // When the return type is the same as the value type, no conversion
            // necessary
            return v;
          else if constexpr(std::is_same_v<T, std::string>)
          {
            // When returning a string...
            if constexpr (std::is_same_v<V, bool>)
              // If the value is a bool, return "true" or "false"
              return v ? std::string("true") : std::string("false");
            else
              // Otherwise the value will be numeric and can be converted to a string
              return std::to_string(v);
          }
          else if constexpr(std::is_same_v<V, std::string>)
          {
            // When the value is a string...
            if constexpr(std::is_same_v<T, bool>)
            {
              // Parse the string to get a boolean.
              //   "1" and "true" will be converted to true
              //   "0" and "false" will be converted to false
              // Other values will throw a std::invalid_argument exception.
              // Parsing is case-insensitive.
              std::string v_lower = cjf::to_lower(v.substr(0, 5));
              if (v_lower == "1" || v_lower == "true")
              {
                return true;
              }
              else if (v.empty() || v_lower == "0" || v_lower == "false")
              {
                return false;
              }
              else
              {
                return std::unexpected(param_error::invalid_cast);
              }
            }
            if constexpr(std::is_integral_v<T> && std::is_signed_v<T>)
            {
              // Parse the string to get a signed integer. If the value is larger or
              // smaller than the range of type T, return param_error::out_of_range.
              const long long int_val = std::stoll(v);
              if (int_val < std::numeric_limits<T>::min() || int_val > std::numeric_limits<T>::max())
              {
                return std::unexpected(param_error::out_of_range);
              }
              return static_cast<T>(int_val);
            }
            else if constexpr(std::is_integral_v<T> && !std::is_signed_v<T>)
            {
              // Return param_error::out_of_range if the value is negative.
              // The check is done on the string because std::stoull() will
              // wrap around a negative value to a large positive value.
              auto n = v.find_first_not_of(" \f\n\r\t\v");
              if (n != std::string::npos && v[n] == '-')
              {
                return std::unexpected(param_error::out_of_range);
              }
              const unsigned long long uint_val = std::stoull(v);
              if (uint_val > std::numeric_limits<T>::max())
              {
                return std::unexpected(param_error::out_of_range);
              }
              return static_cast<T>(uint_val);
            }
            else if constexpr(std::is_floating_point_v<T>)
            {
              // Parse the string to get a floating point number.
              return static_cast<T>(std::stod(v));
            }
            else
            {
              static_assert(always_false_v<V>, "invalid type cast");
            }
          }
          else if constexpr(std::is_integral_v<T> || std::is_floating_point_v<T>)
          {
            // When the return type and value type are both numeric or bool, they
            // can be statically cast.
            return static_cast<T>(v);
          }
          else
          {
            static_assert(always_false_v<T>, "invalid type cast");
          } },
        value);
  }

  template <typename T>
  constexpr std::expected<T, param_error> param_cast(const std::expected<param_value, param_error> &value)
  {
    return value ? param_cast<T>(*value)
                 : std::unexpected(value.error());
  }

  template <typename T>
  constexpr std::expected<T, param_error> param_cast(const param &param)
  {
    return param_cast<T>(param.get());
  }

  inline constexpr bool param::has_value() const noexcept
  {
    return get().has_value();
  }

  template <typename T>
  const std::expected<T, param_error> param::get_as() const
  {
    return param_cast<T>(get());
  }

} // namespace cjf

#endif /* E5346742_E5F1_48E3_9463_6B2868629BC6 */
