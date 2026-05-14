#ifndef A1011013_CD5E_43EC_AFBB_8ABABDCA6D35
#define A1011013_CD5E_43EC_AFBB_8ABABDCA6D35

/**
 * @file modes.h
 * @brief Animation modes for status LEDs
 *
 * This file defines various LED animation modes using the strategy pattern.
 * Each mode implements a specific visual pattern (blink, pulse, rainbow, solid).
 */

#include "cjf/color.h"
#include "cjf/easing.h"
#include <cjf/led_strip.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <memory>
#include <variant>

namespace cjf
{

  /**
   * @brief Abstract base class for LED animation modes
   *
   * Defines the interface for LED animation strategies. Each concrete mode
   * implements tick() to update the LED state based on the current tick count.
   */
  struct status_led_mode_base
  {
    virtual ~status_led_mode_base() = default;

    // Copy and move operations for polymorphic base
    status_led_mode_base() = default;
    status_led_mode_base(const status_led_mode_base &) = default;
    status_led_mode_base(status_led_mode_base &&) = default;
    status_led_mode_base &operator=(const status_led_mode_base &) = default;
    status_led_mode_base &operator=(status_led_mode_base &&) = default;

    /**
     * @brief Update LED state for the current tick
     * @param leds The LED strip to control
     * @param tick_count The current tick number (0, 1, 2, ...)
     * @return Period until the next tick in FreeRTOS ticks, or 0 if no more updates needed
     *
     * Called by status_led on each timer tick. The mode should update the LED
     * state based on tick_count and return when the next update should occur.
     * Returning 0 indicates the animation is complete (useful for one-shot effects)
     * or that no ongoing animation is needed (e.g., solid color).
     */
    virtual TickType_t tick(led_strip &leds, uint32_t tick_count) const noexcept = 0;
  };

  /**
   * @brief Blinking LED animation mode
   *
   * Alternates the LED between two colors with configurable on/off times.
   * Useful for attention-grabbing status indicators (errors, notifications).
   */
  struct status_led_blink : status_led_mode_base
  {
    /**
     * @brief Configuration for blink animation
     */
    struct config
    {
      size_t led = 0;                            ///< LED index to animate
      cjf::color color;                          ///< Color when LED is on
      TickType_t on_time;                        ///< Duration LED stays on
      TickType_t off_time;                       ///< Duration LED stays off
      cjf::color off_color = cjf::colors::black; ///< Color when LED is off (default: black/off)
    };

    /**
     * @brief Construct a blink mode
     * @param config Blink animation parameters
     */
    status_led_blink(const status_led_blink::config &config) noexcept;

    // Copyable and movable
    status_led_blink(const status_led_blink &) = default;
    status_led_blink(status_led_blink &&) = default;
    status_led_blink &operator=(const status_led_blink &) = default;
    status_led_blink &operator=(status_led_blink &&) = default;

    /**
     * @brief Create a blink mode on the heap
     * @param config Blink animation parameters
     * @return Unique pointer to the mode, or nullptr on allocation failure
     *
     * Use this factory for dynamic mode management (e.g., event maps).
     * For static modes, use the constructor directly.
     */
    static std::unique_ptr<status_led_blink> create(const status_led_blink::config &config) noexcept;

    TickType_t tick(led_strip &leds, uint32_t tick_count) const noexcept override;

  private:
    const config config_;
  };

  /**
   * @brief Pulsing LED animation mode
   *
   * Smoothly fades the LED brightness in and out using easing functions.
   * Creates a breathing effect ideal for idle/waiting states.
   */
  struct status_led_pulse : status_led_mode_base
  {
    /**
     * @brief Configuration for pulse animation
     */
    struct config
    {
      size_t led = 0;                               ///< LED index to animate
      cjf::color color;                             ///< Peak color at full brightness
      TickType_t period;                            ///< Full cycle duration (fade in + fade out)
      easing_func easing = easing::linear;          ///< Easing function
      TickType_t tick_interval = pdMS_TO_TICKS(50); ///< Update interval (smaller = smoother)
    };

    /**
     * @brief Construct a pulse mode
     * @param config Pulse animation parameters
     */
    status_led_pulse(const status_led_pulse::config &config) noexcept;

    // Copyable and movable
    status_led_pulse(const status_led_pulse &) = default;
    status_led_pulse(status_led_pulse &&) = default;
    status_led_pulse &operator=(const status_led_pulse &) = default;
    status_led_pulse &operator=(status_led_pulse &&) = default;

    /**
     * @brief Create a pulse mode on the heap
     * @param config Pulse animation parameters
     * @return Unique pointer to the mode, or nullptr on allocation failure
     */
    static std::unique_ptr<status_led_pulse> create(const status_led_pulse::config &config) noexcept;

    TickType_t tick(led_strip &leds, uint32_t tick_count) const noexcept override;

  private:
    const config config_;
  };

  /**
   * @brief Rainbow cycle animation mode
   *
   * Cycles the LED through the full HSV color spectrum. Useful for
   * attracting attention or indicating special states.
   */
  struct status_led_rainbow_cycle : status_led_mode_base
  {
    /**
     * @brief Configuration for rainbow cycle animation
     */
    struct config
    {
      size_t led = 0;                               ///< LED index to animate
      uint8_t brightness = 255;                     ///< LED brightness (0-255)
      uint8_t saturation = 255;                     ///< Color saturation (0-255)
      uint16_t phase = 0;                           ///< Starting hue (0-359 degrees)
      TickType_t period = pdMS_TO_TICKS(2000);      ///< Full rainbow cycle duration
      TickType_t tick_interval = pdMS_TO_TICKS(50); ///< Update interval
    };

    /**
     * @brief Construct a rainbow cycle mode
     * @param config Rainbow animation parameters
     */
    status_led_rainbow_cycle(const status_led_rainbow_cycle::config &config) noexcept;

    // Copyable and movable
    status_led_rainbow_cycle(const status_led_rainbow_cycle &) = default;
    status_led_rainbow_cycle(status_led_rainbow_cycle &&) = default;
    status_led_rainbow_cycle &operator=(const status_led_rainbow_cycle &) = default;
    status_led_rainbow_cycle &operator=(status_led_rainbow_cycle &&) = default;

    /**
     * @brief Create a rainbow cycle mode on the heap
     * @param config Rainbow animation parameters
     * @return Unique pointer to the mode, or nullptr on allocation failure
     */
    static std::unique_ptr<status_led_rainbow_cycle> create(const status_led_rainbow_cycle::config &config) noexcept;

    TickType_t tick(led_strip &leds, uint32_t tick_count) const noexcept override;

  private:
    const config config_;
  };

  /**
   * @brief Solid color LED mode
   *
   * Sets the LED to a constant color without animation. Useful for
   * stable status indicators (connected, ready, etc.).
   */
  struct status_led_solid : status_led_mode_base
  {
    /**
     * @brief Configuration for solid color
     */
    struct config
    {
      size_t led = 0;   ///< LED index to set
      cjf::color color; ///< Color to display
    };

    /**
     * @brief Construct a solid color mode
     * @param config Solid color parameters
     */
    status_led_solid(const status_led_solid::config &config) noexcept;

    // Copyable and movable
    status_led_solid(const status_led_solid &) = default;
    status_led_solid(status_led_solid &&) = default;
    status_led_solid &operator=(const status_led_solid &) = default;
    status_led_solid &operator=(status_led_solid &&) = default;

    /**
     * @brief Create a solid color mode on the heap
     * @param config Solid color parameters
     * @return Unique pointer to the mode, or nullptr on allocation failure
     */
    static std::unique_ptr<status_led_solid> create(const status_led_solid::config &config) noexcept;

    /**
     * @brief Apply solid color to LED
     * @param leds The LED strip to control
     * @param tick_count Current tick number (ignored for solid)
     * @return 0 (no ongoing animation needed)
     *
     * Sets the LED color once and returns 0 since no ongoing
     * animation or timer is required.
     */
    TickType_t tick(led_strip &leds, uint32_t tick_count) const noexcept override;

  private:
    const config config_;
  };

  using status_led_mode = std::variant<
      status_led_blink,
      status_led_pulse,
      status_led_rainbow_cycle,
      status_led_solid>;

} // namespace cjf

#endif /* A1011013_CD5E_43EC_AFBB_8ABABDCA6D35 */
