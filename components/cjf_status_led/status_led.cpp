#include "cjf/status_led.h"
#include <esp_log.h>

namespace cjf
{

  status_led::status_led(cjf::led_strip &led) noexcept
      : current_mode_(nullptr), leds_(led) {}

  void status_led::off() noexcept
  {
    current_mode_.reset();
    leds_.clear();
    leds_.refresh();
  }

  void status_led::set_mode(const status_led_mode* mode) noexcept
  {
    // Stop the current mode and release all resources associated with it
    current_mode_.reset();
    if (mode)
    {
      // Apply the new mode to the LED strip and store the runtime instance
      current_mode_ = mode->apply(leds_);
    }
  }

} // namespace cjf
