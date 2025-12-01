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
#include <cjf/led_strip.h>

namespace cjf
{

  /**
   * @brief Controller for status LED animations
   *
   * Manages a status LED strip with support for dynamic mode switching. Each mode
   * (blink, pulse, rainbow, solid) controls the LED animation until a new mode is set.
   * The controller automatically manages the lifecycle of the current animation.
   *
   * Thread-safety: Methods can be called from any task. Mode switching stops the
   * current animation before starting the new one.
   */
  class status_led
  {
  public:
    /**
     * @brief Construct a status LED controller
     * @param leds Reference to the LED strip to control (must outlive this object)
     */
    explicit status_led(cjf::led_strip &leds) noexcept;

    // Move-only semantics: Each `status_led` holds a reference to an LED strip and
    // manages animation state. Copying would create multiple controllers managing the
    // same LEDs, leading to conflicting animations and undefined behavior.
    status_led(status_led&&) noexcept = default;
    status_led& operator=(status_led&&) noexcept = default;
    status_led(const status_led&) = delete;
    status_led& operator=(const status_led&) = delete;

    /**
     * @brief Turn off the LED and stop any active animation
     *
     * Clears the LED strip and stops the current mode's animation timer.
     */
    void clear() noexcept;

    /**
     * @brief Set a new animation mode
     * @param mode Pointer to the mode to activate, or nullptr to turn off
     *
     * Stops the current animation (if any) and starts the new mode. The mode pointer
     * must remain valid for the duration of its use. Use the constructors to allocate
     * on the stack or as static constants. Use the `create()` factory methods to allocate
     * on the heap.
     *
     * Example with stack-allocated mode:
     * ```cpp
     * static const cjf::status_led_blink STATUSLED_READY({
     *     .led = 0,
     *     .color = cjf::colors::green,
     *     .on_time = pdMS_TO_TICKS(100),
     *     .off_time = pdMS_TO_TICKS(100)
     * });
     * led.set_mode(&STATUSLED_READY);
     * ```
     *
     * Example with heap-allocated mode:
     * ```cpp
     * auto mode = cjf::status_led_blink::create({...});
     * status_led.set_mode(*mode);
     * ```
     */
    void set_mode(const status_led_mode* mode) noexcept;

  private:
    std::unique_ptr<status_led_mode::runtime> current_mode_;
    cjf::led_strip &leds_;
  };

} // namespace cjf

#endif // B8D6BBC7_1FB8_4814_AFC4_A3FEFFDD1965
