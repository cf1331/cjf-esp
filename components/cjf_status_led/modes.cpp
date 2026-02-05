#include "cjf/status_led/modes.h"
#include <esp_log.h>

namespace cjf
{
  static const char *CJF_STATUS_LED = "cjf::status_led";

  void set_pixel(led_strip &strip, const size_t pixel_num, const cjf::color &color)
  {
    std::visit(
        [&](const auto &c)
        {
          using T = std::decay_t<decltype(c)>;
          if constexpr (std::is_same_v<T, cjf::hsv>)
          {
            strip.set_pixel_hsv(pixel_num, c.h, c.s, c.v);
          }
          else if constexpr (std::is_same_v<T, cjf::rgb>)
          {
            strip.set_pixel_rgb(pixel_num, c.r, c.g, c.b);
          }
          else if constexpr (std::is_same_v<T, cjf::rgbw>)
          {
            strip.set_pixel_rgbw(pixel_num, c.r, c.g, c.b, c.w);
          }
        },
        color);
  }

  // ============================================================================
  // status_led_blink
  // ============================================================================

  status_led_blink::status_led_blink(const status_led_blink::config &config) noexcept
      : config_(config) {}

  std::unique_ptr<status_led_blink> status_led_blink::create(const status_led_blink::config &config) noexcept
  {
#ifdef __cpp_exceptions
    try
    {
#endif
      return std::make_unique<status_led_blink>(config);
#ifdef __cpp_exceptions
    }
    catch (...)
    {
      return nullptr;
    }
#endif
  }

  TickType_t status_led_blink::tick(led_strip &leds, uint32_t tick_count) const noexcept
  {
    bool is_on = (tick_count % 2) == 0;
    set_pixel(leds, config_.led, is_on ? config_.color : config_.off_color);
    leds.refresh();
    return is_on ? config_.on_time : config_.off_time;
  }

  // ============================================================================
  // status_led_pulse
  // ============================================================================

  status_led_pulse::status_led_pulse(const status_led_pulse::config &config) noexcept
      : config_(config) {}

  std::unique_ptr<status_led_pulse> status_led_pulse::create(const status_led_pulse::config &config) noexcept
  {
#ifdef __cpp_exceptions
    try
    {
#endif
      return std::make_unique<status_led_pulse>(config);
#ifdef __cpp_exceptions
    }
    catch (...)
    {
      return nullptr;
    }
#endif
  }

  TickType_t status_led_pulse::tick(led_strip &leds, uint32_t tick_count) const noexcept
  {
    // Calculate progress through the cycle (0.0 to 1.0)
    uint32_t ticks_per_period = config_.period / config_.tick_interval;
    float progress = static_cast<float>(tick_count % ticks_per_period) / ticks_per_period;

    // Convert to phase (0.0 -> 1.0 -> 0.0 for fade in/out)
    float phase = (progress < 0.5f) ? (progress * 2.0f) : ((1.0f - progress) * 2.0f);
    float eased = config_.easing(phase);

    auto color_val = config_.color * eased;
    set_pixel(leds, config_.led, color_val);
    leds.refresh();

    return config_.tick_interval;
  }

  // ============================================================================
  // status_led_rainbow_cycle
  // ============================================================================

  status_led_rainbow_cycle::status_led_rainbow_cycle(const status_led_rainbow_cycle::config &config) noexcept
      : config_(config) {}

  std::unique_ptr<status_led_rainbow_cycle> status_led_rainbow_cycle::create(const status_led_rainbow_cycle::config &config) noexcept
  {
#ifdef __cpp_exceptions
    try
    {
#endif
      return std::make_unique<status_led_rainbow_cycle>(config);
#ifdef __cpp_exceptions
    }
    catch (...)
    {
      return nullptr;
    }
#endif
  }

  TickType_t status_led_rainbow_cycle::tick(led_strip &leds, uint32_t tick_count) const noexcept
  {
    // Calculate hue based on tick count
    uint32_t ticks_per_period = config_.period / config_.tick_interval;
    float progress = static_cast<float>(tick_count % ticks_per_period) / ticks_per_period;
    uint16_t hue = (config_.phase + static_cast<uint16_t>(progress * 360.0f)) % 360;

    leds.set_pixel_hsv(config_.led, hue, config_.saturation, config_.brightness);
    leds.refresh();

    return config_.tick_interval;
  }

  // ============================================================================
  // status_led_solid
  // ============================================================================

  status_led_solid::status_led_solid(const status_led_solid::config &config) noexcept
      : config_(config) {}

  std::unique_ptr<status_led_solid> status_led_solid::create(const status_led_solid::config &config) noexcept
  {
#ifdef __cpp_exceptions
    try
    {
#endif
      return std::make_unique<status_led_solid>(config);
#ifdef __cpp_exceptions
    }
    catch (...)
    {
      return nullptr;
    }
#endif
  }

  TickType_t status_led_solid::tick(led_strip &leds, uint32_t tick_count) const noexcept
  {
    set_pixel(leds, config_.led, config_.color);
    leds.refresh();
    return 0; // No ongoing animation needed
  }

} // namespace cjf
