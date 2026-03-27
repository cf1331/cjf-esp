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
   * while (!t.has_expired()) {
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
     * The timeout starts counting immediately from construction.
     */
    timeout(const TickType_t ticks);

    /**
     * @brief Create a timeout from milliseconds
     * @param ms Duration in milliseconds, -1 for `portMAX_DELAY`
     * @return A timeout object with the specified duration
     *
     * Convenience factory method that converts milliseconds to FreeRTOS ticks.
     * Negative values for `ms` will create a timeout with `portMAX_DELAY`, which effectively never expires.
     */
    static timeout from_ms(int32_t ms) noexcept;

    /**
     * @brief Get the timeout duration
     * @return The timeout duration in FreeRTOS ticks
     */
    TickType_t duration() const noexcept;

    /**
     * @brief Get the timeout duration in milliseconds
     * @return The timeout duration in milliseconds, or -1 for portMAX_DELAY
     */
    int32_t duration_ms() const noexcept;

    /**
     * @brief Get the number of ticks elapsed since construction
     * @return Number of ticks that have elapsed
     *
     * This method correctly handles tick counter wraparound. The returned value
     * may exceed the original timeout duration if the timeout has expired.
     */
    TickType_t elapsed() const noexcept;

    /**
     * @brief Get the number of ticks that have elapsed since construction
     * @param now Current tick count to use instead of calling xTaskGetTickCount()
     * @return Number of ticks that have elapsed
     *
     * This overload allows deterministic testing by accepting an external tick count.
     * Useful for unit tests where system time needs to be controlled.
     */
    TickType_t elapsed(TickType_t now) const noexcept;

    /**
     * @brief Get elapsed time in milliseconds
     * @return Elapsed time in milliseconds, or -1 for portMAX_DELAY
     */
    int32_t elapsed_ms() const noexcept;

    /**
     * @brief Get elapsed time in milliseconds
     * @param now Current tick count to use instead of calling xTaskGetTickCount()
     * @return Elapsed time in milliseconds, or -1 for portMAX_DELAY
     *
     * This overload allows deterministic testing by accepting an external tick count.
     */
    int32_t elapsed_ms(TickType_t now) const noexcept;

    /**
     * @brief Check if the timeout has expired
     * @return `true` if the elapsed time equals or exceeds the timeout duration, `false` otherwise
     *
     * This method is equivalent to checking if `remaining() == 0`.
     */
    bool has_expired() const noexcept;

    /**
     * @brief Check if the timeout has expired
     * @param now Current tick count to use instead of calling xTaskGetTickCount()
     * @return `true` if the elapsed time equals or exceeds the timeout duration
     *
     * This overload allows deterministic testing by accepting an external tick count.
     */
    bool has_expired(TickType_t now) const noexcept;

    /**
     * @brief Get the number of ticks remaining until timeout expiration
     * @return Number of ticks remaining, or 0 if the timeout has expired
     *
     * This method returns the number of ticks left before the timeout expires.
     * Once expired, it will always return 0.
     */
    TickType_t remaining() const noexcept;

    /**
     * @brief Get the number of ticks remaining until timeout expiration
     * @param now Current tick count to use instead of calling xTaskGetTickCount()
     * @return Number of ticks remaining, or 0 if the timeout has expired
     *
     * This overload allows deterministic testing by accepting an external tick count.
     */
    TickType_t remaining(TickType_t now) const noexcept;

    /**
     * @brief Get remaining time in milliseconds
     * @return Remaining time in milliseconds, -1 for portMAX_DELAY, or 0 if expired
     */
    int32_t remaining_ms() const noexcept;

    /**
     * @brief Get remaining time in milliseconds
     * @param now Current tick count to use instead of calling xTaskGetTickCount()
     * @return Remaining time in milliseconds, -1 for portMAX_DELAY, or 0 if expired
     *
     * This overload allows deterministic testing by accepting an external tick count.
     */
    int32_t remaining_ms(TickType_t now) const noexcept;

    /**
     * @brief Assign a new timeout duration
     * @param ticks New timeout duration in FreeRTOS ticks
     * @return Reference to this timeout object
     *
     * Updates the timeout duration while keeping the start time unchanged.
     * This allows extending or shortening the timeout without resetting the start point.
     */
    timeout &operator=(TickType_t ticks);

  private:
    TickType_t start_ticks;   ///< Tick count when timeout was constructed
    TickType_t timeout_ticks; ///< Duration of the timeout in ticks
  };

} // namespace cjf

#endif /* AD7ECDEC_93B2_4673_84B4_0708C0F7F016 */
