#ifndef FC53A5AE_B180_4B75_99D3_E595F185E0C9
#define FC53A5AE_B180_4B75_99D3_E595F185E0C9
/**
 * @file inplace_string_param.h
 * @brief String parameter with fixed-size backing buffer.
 *
 * The returned `std::string_view` from `get()` points to the internal buffer
 * and is valid until the next `set()` call or destruction of the param.
 */

#include "cjf/params.h"
#include "cjf/watchable.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <optional>

namespace cjf
{

  /**
   * @brief Mutable string parameter backed by a fixed-size internal buffer.
   *
   * Stores a string value in a fixed-size internal buffer. The buffer capacity is
   * set at compile time via the template parameter `Capacity`. All `param_value`
   * types are automatically converted to their string representation.
   *
   * @tparam Capacity Maximum number of characters (excluding null terminator)
   *
   * @warning The `std::string_view` returned by `get()` is invalidated by any
   *          subsequent call to `set()` or by destruction of this object.
   *
   * @code{.cpp}
   * cjf::inplace_string_param<32> name("ESP32-S3");
   * name.get();                        // std::string_view("ESP32-S3")
   * name.set(std::string_view("other")); // ok
   * name.set(42);                      // converted to "42"
   * name.set(true);                    // converted to "true"
   * name.set(std::monostate{});        // resets to null (has_value() == false)
   * @endcode
   */
  template <size_t Capacity>
  class inplace_string_param : public string_param
  {
  public:
    inplace_string_param(const inplace_string_param &) = delete;
    inplace_string_param &operator=(const inplace_string_param &) = delete;
    inplace_string_param(inplace_string_param &&) = delete;
    inplace_string_param &operator=(inplace_string_param &&) = delete;

    /**
     * @brief Construct with null value
     */
    inplace_string_param() noexcept = default;

    /**
     * @brief Construct with an initial string value
     * @param value Initial string. Silently truncated to Capacity if longer.
     *              Use the set() return value to detect overflow at runtime.
     */
    explicit inplace_string_param(std::string_view value) noexcept;

    /**
     * @brief Construct from a string literal (implicit), enabling `= "..."` initializers.
     * @param value Null-terminated string. Silently truncated to Capacity if longer.
     */
    inplace_string_param(const char *value) noexcept
        : inplace_string_param(std::string_view{value}) {}

    /**
     * @brief Get the maximum capacity of this parameter
     * @return The maximum number of characters that can be stored (excluding null terminator)
     */
    constexpr size_t capacity() const noexcept;

    /**
     * @brief Get the current string value
     * @return `string_view` into the internal buffer, or an empty view if null
     */
    param_value get() const noexcept override;

    /**
     * @brief Get the current string length
     * @return Number of characters currently stored
     */
    constexpr size_t length() const noexcept;

    /**
     * @brief Get the maximum capacity of this parameter (alias for `capacity()`)
     * @return The maximum number of characters that can be stored (excluding null terminator)
     */
    constexpr size_t max_size() const noexcept;

    /**
     * @brief Set the parameter value
     * @param value Any `param_value` type. `std::monostate` clears to null.
     *              Other types are converted to their string representation.
     *              A resulting string longer than Capacity returns `param_error::out_of_range`.
     * @return `param_error::ok` on success, error code otherwise
     */
    param_error set(const param_value &value) override;

    /**
     * @brief Get the current string size (alias for `length()`)
     * @return Number of characters currently stored
     */
    constexpr size_t size() const noexcept;

    const char *get_c_str() const noexcept override;

    /**
     * @brief Register a callback for value change notifications
     * @param callback Function to invoke when value changes
     * @param ctx User context passed to callback
     */
    void watch(value_changed_func callback, void *ctx = nullptr) override;

    /**
     * @brief Unregister a callback
     * @param callback The callback function to remove
     */
    void unwatch(value_changed_func callback) override;

  private:
    std::optional<std::array<char, Capacity + 1>> buffer_ = std::nullopt;
    size_t length_ = 0;
    watchable<param> watchable_;
  };

  /* Implementation ════════════════════════════════════════════════════════════════════════════ */

  template <size_t Capacity>
  inline inplace_string_param<Capacity>::inplace_string_param(std::string_view value) noexcept
  {
    const size_t len = std::min(value.size(), Capacity);
    auto &buf = buffer_.emplace();
    std::copy_n(value.data(), len, buf.data());
    buf[len] = '\0';
    length_ = len;
  }

  template <size_t Capacity>
  inline constexpr size_t inplace_string_param<Capacity>::capacity() const noexcept
  {
    return Capacity;
  }

  template <size_t Capacity>
  inline param_value inplace_string_param<Capacity>::get() const noexcept
  {
    if (!buffer_.has_value())
      return std::monostate{};
    return std::string_view{buffer_->data(), length_};
  }

  template <size_t Capacity>
  inline param_error inplace_string_param<Capacity>::set(const param_value &value)
  {
    if (std::holds_alternative<std::monostate>(value))
    {
      buffer_.reset();
      length_ = 0;
      watchable_.notify(*this);
      return param_error::ok;
    }
    auto &buf = buffer_.emplace();
    auto res = cjf::to_chars(buf.begin(), buf.begin() + Capacity, value);
    if (!res)
    {
      buffer_.reset();
      length_ = 0;
      return res.error();
    }
    length_ = res->size();
    buf[length_] = '\0';
    watchable_.notify(*this);
    return param_error::ok;
  }

  template <size_t Capacity>
  inline void inplace_string_param<Capacity>::watch(value_changed_func callback, void *ctx)
  {
    watchable_.watch(callback, ctx);
  }

  template <size_t Capacity>
  inline void inplace_string_param<Capacity>::unwatch(value_changed_func callback)
  {
    watchable_.unwatch(callback);
  }

  template <size_t Capacity>
  inline constexpr size_t inplace_string_param<Capacity>::length() const noexcept
  {
    return length_;
  }

  template <size_t Capacity>
  inline constexpr size_t inplace_string_param<Capacity>::max_size() const noexcept
  {
    return Capacity;
  }

  template <size_t Capacity>
  inline constexpr size_t inplace_string_param<Capacity>::size() const noexcept
  {
    return length_;
  }

  template <size_t Capacity>
  inline const char *inplace_string_param<Capacity>::get_c_str() const noexcept
  {
    return buffer_.has_value() ? buffer_->data() : nullptr;
  }

} // namespace cjf

#endif /* FC53A5AE_B180_4B75_99D3_E595F185E0C9 */
