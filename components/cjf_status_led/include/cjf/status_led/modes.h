#ifndef A1011013_CD5E_43EC_AFBB_8ABABDCA6D35
#define A1011013_CD5E_43EC_AFBB_8ABABDCA6D35

/**
 * @file modes.h
 * @brief Animation modes for status LEDs
 *
 * This file defines various LED animation modes using the strategy pattern.
 * Each mode implements a specific visual pattern (blink, pulse, rainbow, solid).
 * Animations are generally implemented using FreeRTOS timers.
 */

#include "cjf/color.h"
#include "cjf/easing.h"
#include <cjf/led_strip.h>
#include <freertos/timers.h>
#include <memory>

namespace cjf
{

  /**
   * @brief Abstract base class for LED animation modes
   *
   * Defines the interface for LED animation strategies. Each concrete mode
   * implements apply() to start its animation. Modes can optionally return a 
   * runtime object that manages the animation's lifecycle.
   */
  struct status_led_mode
  {
    /**
     * @brief Runtime state for an active animation (optional)
     *
     * Manages the lifecycle of mode-specific resources (timers, state) for
     * animated modes. Destruction automatically stops the animation and cleans
     * up resources. Modes that only set a static state (e.g., solid color)
     * don't need a runtime and return nullptr from apply().
     */
    struct runtime
    {
      virtual ~runtime() = default;
    };

    /**
     * @brief Apply this mode to an LED strip
     * @param leds The LED strip to control
     * @return Runtime object managing the animation, or nullptr
     *
     * Starts the mode's visual effect and returns a runtime if ongoing animation
     * is needed. The animation continues until the runtime is destroyed.
     * 
     * Returns nullptr in two cases:
     * - Mode doesn't require animation (e.g., solid color - just sets once)
     * - Failure occurred (timer creation or memory allocation failed)
     *
     * Animated modes (blink, pulse, rainbow) return a runtime to manage timers.
     * Static modes (solid) return nullptr after setting the LED state.
     */
    virtual std::unique_ptr<runtime> apply(led_strip &leds) const noexcept = 0;
  };

  /**
   * @brief Blinking LED animation mode
   *
   * Alternates the LED between two colors with configurable on/off times.
   * Useful for attention-grabbing status indicators (errors, notifications).
   */
  struct status_led_blink : status_led_mode
  {
    /**
     * @brief Configuration for blink animation
     */
    struct config
    {
      size_t led;                            ///< LED index to animate
      cjf::color color;                      ///< Color when LED is on
      TickType_t on_time;                    ///< Duration LED stays on
      TickType_t off_time;                   ///< Duration LED stays off
      cjf::color off_color = cjf::colors::black; ///< Color when LED is off (default: black/off)
    };

    /**
     * @brief Construct a blink mode
     * @param config Blink animation parameters
     */
    explicit status_led_blink(const status_led_blink::config &config) noexcept;

    /**
     * @brief Create a blink mode on the heap
     * @param config Blink animation parameters
     * @return Unique pointer to the mode, or nullptr on allocation failure
     *
     * Use this factory for dynamic mode management (e.g., event maps).
     * For static modes, use the constructor directly.
     */
    static std::unique_ptr<status_led_blink> create(const status_led_blink::config &config) noexcept;

    std::unique_ptr<status_led_mode::runtime> apply(led_strip &leds) const noexcept override;

  private:
    const config config_;
  };

  /**
   * @brief Pulsing LED animation mode
   *
   * Smoothly fades the LED brightness in and out using easing functions.
   * Creates a breathing effect ideal for idle/waiting states.
   */
  struct status_led_pulse : status_led_mode
  {
    /**
     * @brief Configuration for pulse animation
     */
    struct config
    {
      size_t led;                                ///< LED index to animate
      cjf::color color;                          ///< Peak color at full brightness
      TickType_t period;                         ///< Full cycle duration (fade in + fade out)
      easing_func easing = easing::linear;       ///< Easing function
      TickType_t tick = pdMS_TO_TICKS(50);       ///< Update interval (smaller = smoother)
    };

    /**
     * @brief Construct a pulse mode
     * @param config Pulse animation parameters
     */
    explicit status_led_pulse(const status_led_pulse::config &config) noexcept;

    /**
     * @brief Create a pulse mode on the heap
     * @param config Pulse animation parameters
     * @return Unique pointer to the mode, or nullptr on allocation failure
     */
    static std::unique_ptr<status_led_pulse> create(const status_led_pulse::config &config) noexcept;

    std::unique_ptr<status_led_mode::runtime> apply(led_strip &leds) const noexcept override;

  private:
    const config config_;
  };

  /**
   * @brief Rainbow cycle animation mode
   *
   * Cycles the LED through the full HSV color spectrum. Useful for
   * attracting attention or indicating special states.
   */
  struct status_led_rainbow_cycle : status_led_mode
  {
    /**
     * @brief Configuration for rainbow cycle animation
     */
    struct config
    {
      size_t led;                                ///< LED index to animate
      uint8_t brightness = 255;                  ///< LED brightness (0-255)
      uint8_t saturation = 255;                  ///< Color saturation (0-255)
      uint16_t phase = 0;                        ///< Starting hue (0-359 degrees)
      TickType_t period = pdMS_TO_TICKS(2000);   ///< Full rainbow cycle duration
      TickType_t tick = pdMS_TO_TICKS(50);       ///< Update interval
    };

    /**
     * @brief Construct a rainbow cycle mode
     * @param config Rainbow animation parameters
     */
    explicit status_led_rainbow_cycle(const status_led_rainbow_cycle::config &config) noexcept;

    /**
     * @brief Create a rainbow cycle mode on the heap
     * @param config Rainbow animation parameters
     * @return Unique pointer to the mode, or nullptr on allocation failure
     */
    static std::unique_ptr<status_led_rainbow_cycle> create(const status_led_rainbow_cycle::config &config) noexcept;

    std::unique_ptr<status_led_mode::runtime> apply(led_strip &leds) const noexcept override;

  private:
    const config config_;
  };

  /**
   * @brief Solid color LED mode
   *
   * Sets the LED to a constant color without animation. Useful for
   * stable status indicators (connected, ready, etc.).
   */
  struct status_led_solid : status_led_mode
  {
    /**
     * @brief Configuration for solid color
     */
    struct config
    {
      size_t led;         ///< LED index to set
      cjf::color color;   ///< Color to display
    };

    /**
     * @brief Construct a solid color mode
     * @param config Solid color parameters
     */
    explicit status_led_solid(const status_led_solid::config &config) noexcept;

    /**
     * @brief Create a solid color mode on the heap
     * @param config Solid color parameters
     * @return Unique pointer to the mode, or nullptr on allocation failure
     */
    static std::unique_ptr<status_led_solid> create(const status_led_solid::config &config) noexcept;

    /**
     * @brief Apply solid color to LED
     * @param leds The LED strip to control
     * @return nullptr (no runtime needed for static color)
     *
     * Sets the LED color once and returns nullptr since no ongoing
     * animation or timer is required.
     */
    std::unique_ptr<status_led_mode::runtime> apply(led_strip &leds) const noexcept override;

  private:
    const config config_;
  };

} // namespace cjf

#endif /* A1011013_CD5E_43EC_AFBB_8ABABDCA6D35 */
