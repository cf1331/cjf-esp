#ifndef B8D6BBC7_1FB8_4814_AFC4_A3FEFFDD1965
#define B8D6BBC7_1FB8_4814_AFC4_A3FEFFDD1965

/**
 * @file status_led.h
 * @brief Status LED controller for managing animated LED patterns
 *
 * This file provides a high-level abstraction for controlling status indicator LEDs
 * with various animation modes (blink, pulse, rainbow, solid). Modes can be switched
 * dynamically and managed through event-driven state changes.
 */

#include "cjf/status_led/modes.h"
#include <cjf/freertos/semaphore.h>
#include <cjf/freertos/timer.h>
#include <cjf/led_strip.h>
#include <concepts>
#include <optional>

namespace cjf
{

  /**
   * @brief Invoke tick() on any variant containing status_led_mode_base-derived types
   * @tparam Variant The variant type containing modes
   * @param mode The variant containing the mode
   * @param leds The LED strip to control
   * @param tick_count The current tick number
   * @return Period until the next tick in FreeRTOS ticks
   *
   * This template function works with any std::variant whose alternatives all
   * implement the status_led_mode_base interface. It allows users to create
   * custom variants with their own mode types.
   */
  template <typename Variant>
  inline TickType_t invoke_tick(const Variant &mode, led_strip &leds, uint32_t tick_count) noexcept
  {
    return std::visit([&leds, tick_count](const auto &m)
                      { return m.tick(leds, tick_count); }, mode);
  }

  /**
   * @brief Concept for types that can be used as LED strips
   *
   * A type satisfies LedStripLike if it can be implicitly converted to cjf::led_strip&.
   * This allows using either cjf::led_strip directly or wrapper types like
   * ASBPM02::status_led_type that provide RAII power management.
   */
  template <typename T>
  concept LedStripLike = std::convertible_to<T &, cjf::led_strip &>;

  /**
   * @brief Controller for status LED animations
   *
   * Manages a status LED strip with support for dynamic mode switching. Each mode
   * (blink, pulse, rainbow, solid) controls the LED animation until a new mode is set.
   * The controller owns a single FreeRTOS timer that drives all animations.
   *
   * Template parameter LedStrip can be cjf::led_strip or any RAII wrapper type
   * that fulfils the LedStripLike concept.
   *
   * Thread-safety: Methods can be called from any task. Mode switching stops the
   * current animation before starting the new one.
   */
  template <LedStripLike LedStrip = cjf::led_strip>
  class status_led
  {
  public:
    /**
     * @brief Construct a status LED controller
     * @param leds LED strip to control (takes ownership)
     */
    explicit status_led(LedStrip leds) noexcept;

    ~status_led() noexcept;

    // Non-copyable, non-movable (mutex/timer are non-movable)
    status_led(status_led &&) = delete;
    status_led &operator=(status_led &&) = delete;
    status_led(const status_led &) = delete;
    status_led &operator=(const status_led &) = delete;

    /**
     * @brief Turn off the LED and stop any active animation
     *
     * Clears the LED strip and stops the animation timer.
     */
    void clear() noexcept;

    /**
     * @brief Set a new animation mode
     * @param mode Pointer to the mode to activate, or nullptr to turn off
     *
     * Stops the current animation (if any) and starts the new mode. The mode pointer
     * must remain valid for the duration of its use.
     *
     * Example with statically-allocated mode:
     * ```cpp
     * static const cjf::status_led_blink STATUSLED_READY({
     *     .led = 0,
     *     .color = cjf::colors::green,
     *     .on_time = pdMS_TO_TICKS(100),
     *     .off_time = pdMS_TO_TICKS(100)
     * });
     * led.set_mode(&STATUSLED_READY);
     * ```
     */
    void set_mode(const status_led_mode_base *mode) noexcept;

  private:
    LedStrip leds_;
    freertos::mutex mutex_;
    std::optional<freertos::timer> timer_;
    const status_led_mode_base *mode_ = nullptr;
    uint32_t tick_count_ = 0;

    void stop_timer() noexcept;
    void schedule_next_tick(TickType_t period) noexcept;
    static void timer_callback(void *context);
  };

  /**
   * @brief Construct a status_led from an expected LED strip
   * @tparam LedStrip LED strip type satisfying LedStripLike
   * @param leds Expected LED strip to wrap; the error is propagated if it contains an error
   * @return Expected status_led on success, or the original error on failure
   *
   * Converts an `std::expected<LedStrip, esp_err_t>` into an
   * `std::expected<status_led<LedStrip>, esp_err_t>`, propagating any error
   * without constructing the controller. Intended for use with board-support
   * methods that already return expected LED resources:
   *
   * ```cpp
   * auto status_led = cjf::status_led_from(board.use_status_led());
   * if (status_led) status_led->set_mode(&STATUSLED_READY);
   * ```
   */
  template <LedStripLike LedStrip>
  std::expected<status_led<LedStrip>, esp_err_t> status_led_from(std::expected<LedStrip, esp_err_t> leds) noexcept;

/* Template implementations ═════════════════════════════════════════════════════════════════════ */

  template <LedStripLike LedStrip>
  status_led<LedStrip>::status_led(LedStrip leds) noexcept
      : leds_(std::move(leds)), mutex_() {}

  template <LedStripLike LedStrip>
  status_led<LedStrip>::~status_led() noexcept
  {
    stop_timer();
    // Block until any running timer callback completes to ensure mode_ and leds_
    // are not accessed after destruction.
    freertos::lock_guard lock(mutex_, portMAX_DELAY);
  }

  template <LedStripLike LedStrip>
  void status_led<LedStrip>::stop_timer() noexcept
  {
    if (timer_)
    {
      timer_->stop();
      // Do NOT destroy (timer_.reset()) here. The timer daemon processes
      // stop/delete commands asynchronously. If we destroy the timer and
      // immediately create a new one via timer_.emplace(), the new timer
      // reuses the same StaticTimer_t buffer. The pending delete command
      // then corrupts the new timer's list items, crashing in vListInsert.
      // Instead, leave the timer stopped and let schedule_next_tick() reuse
      // it via set_period(), which also restarts a dormant timer.
    }
  }

  template <LedStripLike LedStrip>
  void status_led<LedStrip>::schedule_next_tick(TickType_t period) noexcept
  {
    if (period == 0)
    {
      stop_timer();
      return;
    }

    if (!timer_)
    {
      // Use emplace to construct in-place (timer is non-movable)
      timer_.emplace("status_led", period, false, timer_callback, this);
      if (timer_ && *timer_)
      {
        timer_->start();
      }
    }
    else
    {
      timer_->set_period(period);
    }
  }

  template <LedStripLike LedStrip>
  void status_led<LedStrip>::timer_callback(void *context)
  {
    auto *self = static_cast<status_led *>(context);
    if (!self)
      return;

    freertos::lock_guard lock(self->mutex_, 0);
    if (!lock)
      return;

    if (self->mode_)
    {
      cjf::led_strip &strip = self->leds_;
      TickType_t next_period = self->mode_->tick(strip, self->tick_count_);
      self->tick_count_++;

      if (next_period > 0)
      {
        self->timer_->set_period(next_period);
      }
    }
  }

  template <LedStripLike LedStrip>
  void status_led<LedStrip>::clear() noexcept
  {
    freertos::lock_guard lock(mutex_);

    stop_timer();
    mode_ = nullptr;
    tick_count_ = 0;

    cjf::led_strip &strip = leds_;
    strip.clear();
    strip.refresh();
  }

  template <LedStripLike LedStrip>
  void status_led<LedStrip>::set_mode(const status_led_mode_base *mode) noexcept
  {
    freertos::lock_guard lock(mutex_);

    // Stop existing animation
    stop_timer();
    mode_ = mode;
    tick_count_ = 0;

    if (mode_)
    {
      // Perform first tick immediately and get period for next
      cjf::led_strip &strip = leds_;
      TickType_t next_period = mode_->tick(strip, tick_count_);
      tick_count_++;

      // Schedule next tick if needed
      if (next_period > 0)
      {
        schedule_next_tick(next_period);
      }
    }
  }

  template <LedStripLike LedStrip>
  std::expected<status_led<LedStrip>, esp_err_t> status_led_from(std::expected<LedStrip, esp_err_t> leds) noexcept
  {
    if (leds)
    {
      return std::expected<status_led<LedStrip>, esp_err_t>(std::in_place, std::move(*leds));
    }
    else
    {
      return std::unexpected(leds.error());
    }
  }

} // namespace cjf

#endif // B8D6BBC7_1FB8_4814_AFC4_A3FEFFDD1965
