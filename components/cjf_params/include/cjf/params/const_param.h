#ifndef E12C0F9B_8C8D_4481_B6CC_295BE94B8EE6
#define E12C0F9B_8C8D_4481_B6CC_295BE94B8EE6

/**
 * @file const_param.h
 * @brief Read-only parameter implementation
 */

#include "cjf/params.h"
#include <expected>

namespace cjf
{
  /**
   * @brief Immutable parameter wrapper for read-only configuration values
   *
   * Stores a constant value that cannot be modified after construction.
   * Attempting to call `set()` will return `param_error::read_only` and
   * generate a compiler warning.
   *
   * Use for:
   * - Hardware capabilities (e.g., sensor resolution, chip ID)
   * - Build-time configuration values
   * - Immutable system constants
   *
   * @code{.cpp}
   * const_param chip_id(std::string("ESP32-S3"));
   * chip_id.set(123);  // Returns param_error::read_only (with compiler warning)
   * auto id = chip_id.get_as<std::string>();  // "ESP32-S3"
   * @endcode
   */
  class const_param : public param
  {
  public:
    /**
     * @brief Construct a const_param with an initial value
     * @param value The immutable value to store
     */
    const_param(const param_value& value);

    /**
     * @brief Get the constant value
     * @return The stored value
     */
    param_value get() const noexcept override;

    /**
     * @brief Attempt to set the value (always fails)
     * @param value Ignored
     * @return Always returns `param_error::read_only`
     * @warning Generates compiler warning when called
     */
      __attribute__((warning("the param value is read-only. const_param::set() will always return param_error::read_only")))
    param_error set(const param_value &value) override;

    /**
     * @brief Register a value change callback (no-op for const_param)
     * @param callback Ignored (value never changes)
     * @param ctx Ignored
     * @note Since the value never changes, callbacks are never invoked
     */
    void watch(value_changed_func callback, void *ctx = nullptr) override;

    /**
     * @brief Unregister a callback (no-op for const_param)
     * @param callback Ignored
     */
    void unwatch(value_changed_func callback) override;

  private:
    const param_value value_;
  };

}

#endif /* E12C0F9B_8C8D_4481_B6CC_295BE94B8EE6 */
