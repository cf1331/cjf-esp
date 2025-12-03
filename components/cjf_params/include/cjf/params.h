#ifndef E5346742_E5F1_48E3_9463_6B2868629BC6
#define E5346742_E5F1_48E3_9463_6B2868629BC6

/**
 * @file params.h
 * @brief Type-safe parameter wrapper with automatic type conversion and serialization
 *
 * Provides a polymorphic parameter system for wrapping primitive types and strings,
 * with automatic type conversions and observer pattern support. Designed for
 * configuration management, network serialization, and value binding scenarios.
 */

#include <cjf/string.h>
#include <charconv>
#include <cstdint>
#include <expected>
#include <string>
#include <optional>
#include <variant>

namespace cjf
{
  /// @cond INTERNAL
  template <class>
  inline constexpr bool always_false_v = false;
  /// @endcond

  /**
   * @brief Error codes for parameter operations
   */
  enum param_error
  {
    ok = 0,           ///< Operation succeeded
    read_only,        ///< Attempted to modify a read-only parameter
    invalid_cast,     ///< Type conversion failed (e.g., "abc" to int)
    out_of_range,     ///< Value outside valid range for target type
  };

  /**
   * @brief Variant type holding all supported parameter value types
   *
   * Supports all integer types, bool, float, double, and string.
   * std::monostate represents a null/empty parameter value.
   */
  using param_value = std::variant<
      std::monostate,
      char,
      signed char,
      unsigned char,
      short,
      unsigned short,
      int,
      unsigned int,
      long,
      unsigned long,
      long long,
      unsigned long long,
      bool,
      float,
      double,
      std::string>;

  /**
   * @brief Sentinel value representing a parameter with no value
   */
  inline const param_value param_null = std::monostate{};

  /**
   * @brief Abstract base class for type-safe parameter wrappers
   *
   * Provides a polymorphic interface for parameter values with automatic type
   * conversion, validation, and observer notifications. Thread-safe when using
   * derived implementations with proper synchronization.
   *
   * @note Implementations: See `const_param` for immutable values and
   *       `mutable_param<T>` for mutable values with type constraints.
   */
  class param
  {
  public:
    /**
     * @brief Type alias for null parameter value (std::monostate)
     */
    using null_type = std::monostate;

    /**
     * @brief Callback function type for value change notifications
     * @param p Reference to the parameter that changed
     * @param ctx User-provided context pointer passed to watch()
     */
    using value_changed_func = void (*)(param &p, void *ctx);

    virtual ~param() = default;

    /**
     * @brief Get the current parameter value
     * @return The parameter value
     */
    virtual param_value get() const noexcept = 0;

    /**
     * @brief Set the parameter value
     * @param value New value to store
     * @return `param_error::ok` on success, error code otherwise
     */
    virtual param_error set(const param_value &value) = 0;

    /**
     * @brief Register a callback to be notified when the value changes
     * @param callback Function to call on value changes
     * @param ctx Optional user context passed to callback
     */
    virtual void watch(value_changed_func callback, void *ctx = nullptr) = 0;

    /**
     * @brief Unregister a previously registered callback
     * @param callback The callback function to remove
     */
    virtual void unwatch(value_changed_func callback) = 0;

    /**
     * @brief Get the value converted to a specific type
     * @tparam T Target type for conversion
     * @return Converted value or error if conversion fails
     *
     * @code{.cpp}
     * mutable_param<int> param(42);
     * auto str = param.get_as<std::string>();  // Returns "42"
     * auto val = param.get_as<double>();        // Returns 42.0
     * @endcode
     */
    template <typename T>
    std::expected<T, param_error> get_as() const;

    /**
     * @brief Check if the parameter has a value
     * @return `true` if value is set, `false` if no_value
     */
    constexpr bool has_value() const noexcept;
  };

  /**
   * @brief Convert a param_value to a specific type with automatic conversion
   * @tparam T Target type for conversion
   * @param value The variant value to convert
   * @return Converted value or error code
   *
   * Supports conversions between:
   * - Numeric types (with range checking)
   * - String to/from numeric and bool
   * - Bool to/from string ("true"/"false", "1"/"0")
   *
   * @note String parsing is case-insensitive. Uses `std::from_chars` for
   *       exception-free parsing.
   *
   * @code{.cpp}
   * param_value v = 42;
   * auto str = param_cast<std::string>(v);  // "42"
   * auto dbl = param_cast<double>(v);       // 42.0
   *
   * param_value s = std::string("200");
   * auto i = param_cast<int>(s);            // 200
   * auto too_big = param_cast<int8_t>(s);   // param_error::out_of_range
   * @endcode
   */
  template <typename T>
  constexpr std::expected<T, param_error> param_cast(const param_value &value)
  {
    return std::visit(
        [](auto &&v) -> std::expected<T, param_error>
        {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<V, param::null_type>)
          {
            // null_type is not convertible to any type
            return std::unexpected(param_error::invalid_cast);
          }
          else if constexpr (std::is_same_v<T, V>)
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
              long long int_val;
              auto result = std::from_chars(v.data(), v.data() + v.size(), int_val);
              if (result.ec == std::errc::invalid_argument)
              {
                return std::unexpected(param_error::invalid_cast);
              }
              if (result.ec == std::errc::result_out_of_range ||
                  int_val < std::numeric_limits<T>::min() ||
                  int_val > std::numeric_limits<T>::max())
              {
                return std::unexpected(param_error::out_of_range);
              }
              return static_cast<T>(int_val);
            }
            else if constexpr(std::is_integral_v<T> && !std::is_signed_v<T>)
            {
              // Return param_error::out_of_range if the value is negative.
              // The check is done on the string because std::from_chars will
              // wrap around a negative value to a large positive value.
              auto n = v.find_first_not_of(" \f\n\r\t\v");
              if (n != std::string::npos && v[n] == '-')
              {
                return std::unexpected(param_error::out_of_range);
              }
              unsigned long long uint_val;
              auto result = std::from_chars(v.data(), v.data() + v.size(), uint_val);
              if (result.ec == std::errc::invalid_argument)
              {
                return std::unexpected(param_error::invalid_cast);
              }
              if (result.ec == std::errc::result_out_of_range ||
                  uint_val > std::numeric_limits<T>::max())
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

  // /**
  //  * @brief Convert an expected param_value to a specific type
  //  * @tparam T Target type for conversion
  //  * @param value Expected containing value or error
  //  * @return Converted value or propagated error
  //  */
  // template <typename T>
  // constexpr std::expected<T, param_error> param_cast(const std::expected<param_value, param_error> &value)
  // {
  //   return value ? param_cast<T>(*value)
  //                : std::unexpected(value.error());
  // }

  /**
   * @brief Convert a param's value to a specific type
   * @tparam T Target type for conversion
   * @param param The parameter to extract and convert
   * @return Converted value or error
   */
  template <typename T>
  constexpr std::expected<T, param_error> param_cast(const param &param)
  {
    return param_cast<T>(param.get());
  }

  inline constexpr bool param::has_value() const noexcept
  {
    return !std::holds_alternative<null_type>(get());
  }

  template <typename T>
  std::expected<T, param_error> param::get_as() const
  {
    return param_cast<T>(get());
  }

  template <typename T>
  param_error try_set_from_param(T& target, const param& source)
  {
    auto value = source.get_as<T>();
    if (!value)
    {
      return value.error();
    }
    target = *value;
    return param_error::ok;
  }

} // namespace cjf

#endif /* E5346742_E5F1_48E3_9463_6B2868629BC6 */
