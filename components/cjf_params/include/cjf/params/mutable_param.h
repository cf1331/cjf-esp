#ifndef BA7523FD_3010_4D83_AEB0_11C7791E6112
#define BA7523FD_3010_4D83_AEB0_11C7791E6112

/**
 * @file mutable_param.h
 * @brief Type-constrained mutable parameter implementation
 */

#include "cjf/params.h"
#include "cjf/watchable.h"
#include <expected>

namespace cjf
{
  /**
   * @brief Mutable parameter with type validation and change notifications
   *
   * Stores a value that can be modified with automatic type validation against
   * the template parameter `T`. Notifies registered observers when the value changes.
   *
   * @tparam T The constrained type for this parameter. All `set()` operations
   *           will validate that the value is convertible to `T`.
   *
   * @note Thread-safe: Uses FreeRTOS mutex for synchronization across tasks.
   *
   * Use for:
   * - Runtime configuration values
   * - Sensor readings that trigger UI updates
   * - Network-synchronized state
   *
   * @code{.cpp}
   * // Temperature parameter constrained to double
   * mutable_param<double> temp(25.0);
   *
   * // Register callback for changes
   * temp.watch([](param& p, void* ctx) {
   *     ESP_LOGI("TEMP", "Changed to: %.2f", *p.get_as<double>());
   * });
   *
   * temp.set(30.5);  // Triggers callback
   * temp.set(std::string("32.7"));  // Converts string to double, triggers callback
   * temp.set(std::string("invalid"));  // Returns param_error::invalid_cast
   * @endcode
   */
  template <typename T>
  class mutable_param : public param
  {
  public:
    /**
     * @brief Construct a mutable_param with an optional initial value
     * @param value Initial value, or `param_null` for no value
     */
    mutable_param(const std::expected<param_value, param_error> &value = param_null);

    /**
     * @brief Construct a mutable_param with an initial value
     * @param value Initial value
     */
    template <typename U>
      requires std::convertible_to<U, T>
    mutable_param(U &&value);

    /**
     * @brief Construct a mutable_param with no value (from param_null)
     * @param err Unexpected error (typically param_null)
     */
    mutable_param(const std::unexpected<param_error> &err);

    /**
     * @brief Get the current value
     * @return The stored value or error state
     */
    std::expected<param_value, param_error> get() const noexcept override;

    /**
     * @brief Set the parameter value with type validation
     * @param value New value to store
     * @return `param_error::ok` on success, error code if type conversion fails
     *
     * The value is first validated to ensure it's convertible to type `T`.
     * If successful, the value is stored and all registered watchers are notified.
     */
    param_error set(const std::expected<param_value, param_error> &value) override;

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
    std::expected<param_value, param_error> value_;
    watchable<param> watchable_;
  };

  template <typename T>
  inline mutable_param<T>::mutable_param(const std::expected<param_value, param_error> &value)
      : value_(value)
  {
  }

  template <typename T>
  template <typename U>
    requires std::convertible_to<U, T>
  inline mutable_param<T>::mutable_param(U &&value)
      : value_(param_value(T(std::forward<U>(value))))
  {
  }

  template <typename T>
  inline mutable_param<T>::mutable_param(const std::unexpected<param_error> &err)
      : value_(err)
  {
  }

  template <typename T>
  inline std::expected<param_value, param_error> mutable_param<T>::get() const noexcept
  {
    return value_;
  }

  template <typename T>
  inline param_error mutable_param<T>::set(const std::expected<param_value, param_error> &value)
  {
    // Ensure that value is castable to T
    auto new_value = param_cast<T>(value);
    if (!new_value)
    {
      return new_value.error();
    }
    value_ = new_value;
    watchable_.notify(*this);
    return param_error::ok;
  }

  template <typename T>
  inline void mutable_param<T>::watch(value_changed_func callback, void *ctx)
  {
    watchable_.watch(callback, ctx);
  }

  template <typename T>
  inline void mutable_param<T>::unwatch(value_changed_func callback)
  {
    watchable_.unwatch(callback);
  }

} // namespace cjf

#endif /* BA7523FD_3010_4D83_AEB0_11C7791E6112 */
