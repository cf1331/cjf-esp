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

#include <charconv>
#include <cjf/string.h>
#include <cjf/type_traits.h>
#include <cstdint>
#include <expected>
#include <magic_enum/magic_enum.hpp>
#include <string_view>
#include <variant>

namespace cjf
{
  /**
   * @brief Error codes for parameter operations
   */
  enum param_error
  {
    ok = 0,       ///< Operation succeeded
    read_only,    ///< Attempted to modify a read-only parameter
    invalid_cast, ///< Type conversion failed (e.g., "abc" to int)
    out_of_range, ///< Value outside valid range for target type
  };

  [[nodiscard]] constexpr auto param_error_to_name(param_error err)
  {
    switch (err)
    {
    case param_error::ok:
      return "param_error::ok";
    case param_error::read_only:
      return "param_error::read_only";
    case param_error::invalid_cast:
      return "param_error::invalid_cast";
    case param_error::out_of_range:
      return "param_error::out_of_range";
    default:
      return "param_error::unknown";
    }
  };

  /**
   * @brief Sentinel value representing a parameter with no value
   */
  inline constexpr std::monostate param_null{};

  using param_null_type = std::monostate;

  /**
   * @brief Variant type holding all supported parameter value types
   *
   * Supports all integer types, bool, float, double, and string.
   * std::monostate represents a null/empty parameter value.
   */
  using param_value = std::variant<
      param_null_type,
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
      std::string_view>;

  /**
   * @brief Concept: type must be one of the types in param_value variant
   */
  template <typename T>
  concept param_value_type = is_variant_alternative<T, param_value>::value;


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
     * auto str = param.get_as<std::string_view>();  // Returns "42"
     * auto val = param.get_as<double>();            // Returns 42.0
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
   * @brief Abstract base class for string parameters that can be passed to C APIs.
   *
   * Extends `param` with a `get_c_str()` method guaranteeing a null-terminated
   * pointer, suitable for use with C APIs that require `const char *`.
   */
  class string_param : public param
  {
  public:
    /**
     * @brief Get a null-terminated C string pointer for use with C APIs.
     * @return Pointer to the string value as a null-terminated C string,
     *         or `nullptr` if the parameter is null. Always check `has_value()`
     *         before passing to C APIs that do not accept `nullptr`.
     */
    virtual const char *get_c_str() const noexcept = 0;
  };

  /**
   * @brief Convert a param_value to a specific type with automatic conversion
   * @tparam T Target type for conversion
   * @param value The variant value to convert
   * @return Converted value or error code
   *
   * Supports conversions between:
   * - Numeric types (with range checking)
   * - std::string_view to numeric and bool (parsing)
   *
   * Numeric and bool to std::string_view conversion i not supported — use cjf::to_chars instead
   *
   * @note String parsing is case-insensitive for bool. Uses `std::from_chars` for
   *       exception-free numeric parsing.
   *
   * @code{.cpp}
   * param_value v = 42;
   * auto dbl = param_cast<double>(v);                   // 42.0
   *
   * param_value s = std::string_view("200");
   * auto i = param_cast<int>(s);                        // 200
   * auto too_big = param_cast<int8_t>(s);               // param_error::out_of_range
   *
   * auto sv = param_cast<std::string_view>(v);          // param_error::invalid_cast
   * // use cjf::to_chars(buf, buf+N, v) to produce a string
   * @endcode
   */
  template <typename T>
  constexpr std::expected<T, param_error> param_cast(const param_value &value)
  {
    return std::visit(
        [](auto &&v) -> std::expected<T, param_error>
        {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, V>)
          {
            // When the return type matches the held type, no conversion is needed
            return v;
          }
          else if constexpr (std::is_same_v<V, param::null_type>)
          {
            // Null cannot be converted to any other type. Null represents the absense of a value,
            // so conceptually it doesn't make sense for it to be converted to something else.
            return std::unexpected(param_error::invalid_cast);
          }
          else if constexpr (std::is_same_v<T, std::string_view>)
          {
            // std::string_view cannot be converted from other types. This is because it
            // requires backing storage and we don't have a way to provide that here without
            // dynamic allocation.
            return std::unexpected(param_error::invalid_cast);
          }
          else if constexpr (std::is_same_v<V, std::string_view>)
          {
            // Parsing: string_view -> T
            if constexpr (std::is_same_v<T, bool>)
            {
              bool is_truthy = cjf::case_insensitive_equal(v, "1") || cjf::case_insensitive_equal(v, "true");
              if (is_truthy) return true;
              bool is_falsy = v.empty() || cjf::case_insensitive_equal(v, "0") || cjf::case_insensitive_equal(v, "false");
              if (is_falsy) return false;
              return std::unexpected(param_error::invalid_cast);
            }
            else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>)
            {
              T val;
              auto result = std::from_chars(v.data(), v.data() + v.size(), val);
              if (result.ec == std::errc())
                return val;
              if (result.ec == std::errc::result_out_of_range)
                return std::unexpected(param_error::out_of_range);
              return std::unexpected(param_error::invalid_cast);
            }
            // else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
            // {
            //   long long int_val;
            //   auto result = std::from_chars(v.data(), v.data() + v.size(), int_val);
            //   if (result.ec == std::errc::invalid_argument)
            //     return std::unexpected(param_error::invalid_cast);
            //   if (result.ec == std::errc::result_out_of_range ||
            //       int_val < std::numeric_limits<T>::min() ||
            //       int_val > std::numeric_limits<T>::max())
            //     return std::unexpected(param_error::out_of_range);
            //   return static_cast<T>(int_val);
            // }
            // else if constexpr (std::is_integral_v<T> && !std::is_signed_v<T>)
            // {
            //   // Check for leading '-' before from_chars, which would wrap silently
            //   auto n = v.find_first_not_of(" \f\n\r\t\v");
            //   if (n != std::string_view::npos && v[n] == '-')
            //     return std::unexpected(param_error::out_of_range);
            //   unsigned long long uint_val;
            //   auto result = std::from_chars(v.data(), v.data() + v.size(), uint_val);
            //   if (result.ec == std::errc::invalid_argument)
            //     return std::unexpected(param_error::invalid_cast);
            //   if (result.ec == std::errc::result_out_of_range ||
            //       uint_val > std::numeric_limits<T>::max())
            //     return std::unexpected(param_error::out_of_range);
            //   return static_cast<T>(uint_val);
            // }
            // else if constexpr (std::is_floating_point_v<T>)
            // {
            //   double fp_val;
            //   auto result = std::from_chars(v.data(), v.data() + v.size(), fp_val);
            //   if (result.ec == std::errc::invalid_argument)
            //     return std::unexpected(param_error::invalid_cast);
            //   if (result.ec == std::errc::result_out_of_range)
            //     return std::unexpected(param_error::out_of_range);
            //   return static_cast<T>(fp_val);
            // }
            else
            {
              static_assert(always_false_v<T>, "unsupported target type for string_view param_cast");
              return std::unexpected(param_error::invalid_cast);
            }
          }
          else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>)
          {
            // Numeric <-> numeric: static cast
            return static_cast<T>(v);
          }
          else
          {
            static_assert(always_false_v<T>, "invalid type cast");
            return std::unexpected(param_error::invalid_cast);
          }
        },
        value);
  }

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
  param_error try_set_from_param(T &target, const param &source)
  {
    auto value = source.get_as<T>();
    if (!value)
    {
      return value.error();
    }
    target = *value;
    return param_error::ok;
  }

  /**
   * @brief Convert a param_value to its string representation into a caller-provided buffer
   *
   * Mirrors the `std::to_chars` buffer idiom. The returned `string_view` points into
   * `[first, last)` and is valid for as long as the buffer remains in scope.
   *
   * - Numeric types delegate to `std::to_chars`
   * - `bool` writes `"true"` or `"false"`
   * - `std::string_view` copies the string data into the buffer
   * - `std::monostate` (null) writes nothing and returns an empty view
   *
   * @param first Pointer to start of output buffer
   * @param last  Pointer one-past-end of output buffer
   * @param value The param_value to convert
   * @return The written string as a `string_view` into the buffer,
   *         or `param_error::out_of_range` if the buffer is too small
   */
  [[nodiscard]] std::expected<std::string_view, param_error>
  to_chars(char *first, char *last, const param_value &value) noexcept;

} // namespace cjf

#endif /* E5346742_E5F1_48E3_9463_6B2868629BC6 */
