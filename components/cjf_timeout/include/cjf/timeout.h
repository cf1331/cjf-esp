#ifndef AD7ECDEC_93B2_4673_84B4_0708C0F7F016
#define AD7ECDEC_93B2_4673_84B4_0708C0F7F016

/**
 * @file timeout.h
 * @brief FreeRTOS timeout tracking utility
 *
 * This file provides a C++ class for tracking timeouts using FreeRTOS tick counts.
 * The timeout class captures the current tick count at construction and provides
 * methods to check expiration and query elapsed/remaining time.
 */

#include <freertos/FreeRTOS.h>

namespace cjf
{
  /**
   * @brief Timeout tracking class for FreeRTOS
   *
   * Provides timeout tracking functionality using FreeRTOS tick counts. The timeout
   * starts counting from the moment the object is constructed. This class correctly
   * handles tick counter wraparound through unsigned arithmetic.
   *
   * @note This class is copyable and lightweight - it only stores two tick values.
   *
   * Example usage:
   * ```
   * timeout t(pdMS_TO_TICKS(1000)); // 1 second timeout
   * while (!t.is_expired()) {
   *   // Do work...
   *   if (work_complete) break;
   * }
   * ```
   */
  class timeout
  {
  public:
    /**
     * @brief Construct a timeout with the specified duration
     * @param ticks Number of FreeRTOS ticks before the timeout expires
     *
     * The timeout starts counting immediately from construction. Use `pdMS_TO_TICKS()`
     * to convert milliseconds to ticks.
     */
    timeout(TickType_t ticks);

    /**
     * @brief Check if the timeout has expired
     * @return `true` if the elapsed time equals or exceeds the timeout duration, `false` otherwise
     *
     * This method is equivalent to checking if `remaining() == 0`.
     */
    bool is_expired() const;

    /**
     * @brief Get the number of ticks elapsed since timeout construction
     * @return Number of ticks that have elapsed
     *
     * This method correctly handles tick counter wraparound. The returned value
     * may exceed the original timeout duration if the timeout has expired.
     */
    TickType_t elapsed() const;

    /**
     * @brief Get the number of ticks remaining until timeout expiration
     * @return Number of ticks remaining, or 0 if the timeout has expired
     *
     * This method returns the number of ticks left before the timeout expires.
     * Once expired, it will always return 0.
     */
    TickType_t remaining() const;

  private:
    TickType_t start_ticks;   ///< Tick count when timeout was constructed
    TickType_t timeout_ticks; ///< Duration of the timeout in ticks
  };

} // namespace cjf

#endif /* AD7ECDEC_93B2_4673_84B4_0708C0F7F016 */
