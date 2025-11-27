#include "cjf/status_led/modes.h"
#include <esp_log.h>
#include <freertos/semphr.h>

namespace cjf
{
  static const char *CJF_STATUS_LED = "cjf::status_led";

  void set_pixel(const led_strip &strip, const size_t pixel_num, const cjf::color &color)
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

  // Base class for timer-based LED modes that handles mutex and timer lifecycle
  struct timer_based_runtime : status_led_mode::runtime
  {
    TimerHandle_t timer;
    SemaphoreHandle_t mutex;
    volatile bool is_active;

    timer_based_runtime(TimerHandle_t timer)
        : timer(timer), mutex(xSemaphoreCreateMutex()), is_active(true) {}

    virtual ~timer_based_runtime()
    {
      is_active = false;

      // Clean up the timer
      if (timer)
      {
        // Take the mutex to ensure callback isn't running
        if (mutex)
        {
          xSemaphoreTake(mutex, portMAX_DELAY);
        }
        // Stop the timer first to prevent new callbacks from being scheduled
        xTimerStop(timer, portMAX_DELAY);
        // Clear the timer ID so any in-flight callbacks will exit early
        vTimerSetTimerID(timer, nullptr);
        // Delete the timer
        xTimerDelete(timer, portMAX_DELAY);
        timer = nullptr;
        // Release mutex
        if (mutex)
        {
          xSemaphoreGive(mutex);
        }
      }

      // Delete mutex
      if (mutex)
      {
        vSemaphoreDelete(mutex);
      }
    }

    // Helper for callbacks to safely acquire the mutex
    bool try_lock()
    {
      if (!is_active)
        return false;
      return xSemaphoreTake(mutex, 0) == pdTRUE;
    }

    void unlock()
    {
      xSemaphoreGive(mutex);
    }

    // Factory method to create and start a timer-based runtime
    template <typename RuntimeType, typename ConfigType>
    static std::unique_ptr<RuntimeType> create(
        const char *timer_name,
        TimerCallbackFunction_t callback,
        TickType_t period,
        const ConfigType *config,
        led_strip &leds) noexcept
    {
      TimerHandle_t timer = xTimerCreate(
          timer_name,
          period,
          pdTRUE,
          nullptr,
          callback);
      if (!timer)
      {
        ESP_LOGE(CJF_STATUS_LED, "Failed to create timer for %s", timer_name);
        return nullptr;
      }
#ifdef __cpp_exceptions
      try
      {
#endif
        auto runtime = std::make_unique<RuntimeType>(config, leds, timer);
        vTimerSetTimerID(timer, runtime.get());
        xTimerStart(timer, 0);
        // Force the first tick to occur immediately
        callback(timer);
        return runtime;
#ifdef __cpp_exceptions
      }
      catch (...)
      {
        // Clean up timer if runtime allocation fails
        xTimerDelete(timer, portMAX_DELAY);
        ESP_LOGE(CJF_STATUS_LED, "Failed to allocate runtime for %s", timer_name);
        return nullptr;
      }
#endif
    }
  };

  struct status_led_blink_runtime : timer_based_runtime
  {
    const status_led_blink::config *config;
    size_t count;
    led_strip &leds;

    status_led_blink_runtime(const status_led_blink::config *config, led_strip &leds, TimerHandle_t timer)
        : timer_based_runtime(timer), config(config), count(0), leds(leds) {}

    static void timer_callback(TimerHandle_t timer)
    {
      auto *self = static_cast<status_led_blink_runtime *>(pvTimerGetTimerID(timer));
      if (!self || !self->try_lock())
        return;

      if (self->count % 2 == 0)
      {
        set_pixel(self->leds, self->config->led, self->config->color);
        xTimerChangePeriod(timer, self->config->on_time, 0);
      }
      else
      {
        set_pixel(self->leds, self->config->led, self->config->off_color);
        xTimerChangePeriod(timer, self->config->off_time, 0);
      }
      self->count++;
      self->leds.refresh();

      self->unlock();
    }
  };

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

  std::unique_ptr<status_led_mode::runtime> status_led_blink::apply(led_strip &leds) const noexcept
  {
    return timer_based_runtime::create<status_led_blink_runtime>(
        "status_led_blink_timer",
        status_led_blink_runtime::timer_callback,
        config_.on_time,
        &config_,
        leds);
  }

  struct status_led_pulse_runtime : timer_based_runtime
  {
    const status_led_pulse::config *config;
    const led_strip &leds;

    float progress;
    const float progress_per_tick;

    status_led_pulse_runtime(const status_led_pulse::config *config, const led_strip &leds, TimerHandle_t timer)
        : timer_based_runtime(timer), config(config), leds(leds), progress(0),
          progress_per_tick(static_cast<float>(config->tick) / config->period) {}

    static void timer_callback(TimerHandle_t timer)
    {
      auto *self = static_cast<status_led_pulse_runtime *>(pvTimerGetTimerID(timer));
      if (!self || !self->try_lock())
        return;

      const auto *cfg = self->config;
      float phase = (self->progress < 0.5f) ? (self->progress * 2.0f) : ((1.0f - self->progress) * 2.0f);
      float eased = cfg->easing(phase);
      // ESP_LOGI(CJF_STATUS_LED, "progress: %f, phase: %f, eased: %f", self->progress, phase, eased);
      auto color_val = cfg->color * eased;
      set_pixel(self->leds, cfg->led, color_val);
      self->leds.refresh();
      self->progress += self->progress_per_tick;
      if (self->progress >= 1.0f)
        self->progress -= 1.0f;

      self->unlock();
    }
  };

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

  std::unique_ptr<status_led_mode::runtime> status_led_pulse::apply(led_strip &leds) const noexcept
  {
    return timer_based_runtime::create<status_led_pulse_runtime>(
        "status_led_pulse_timer",
        status_led_pulse_runtime::timer_callback,
        config_.tick,
        &config_,
        leds);
  }

  struct status_led_rainbow_cycle_runtime : timer_based_runtime
  {
    const status_led_rainbow_cycle::config *config;
    led_strip &leds;

    uint16_t phase;
    uint16_t phase_step;

    status_led_rainbow_cycle_runtime(const status_led_rainbow_cycle::config *config, led_strip &leds, TimerHandle_t timer)
        : timer_based_runtime(timer), config(config), leds(leds),
          phase(config->phase), phase_step(360 * config->tick / config->period) {}

    static void timer_callback(TimerHandle_t timer)
    {
      auto *self = static_cast<status_led_rainbow_cycle_runtime *>(pvTimerGetTimerID(timer));
      if (!self || !self->try_lock())
        return;

      const auto *cfg = self->config;
      self->leds.set_pixel_hsv(cfg->led, self->phase, cfg->saturation, cfg->brightness);
      self->phase += self->phase_step;
      if (self->phase >= 360)
        self->phase -= 360;
      self->leds.refresh();

      self->unlock();
    }
  };

  status_led_rainbow_cycle::status_led_rainbow_cycle(const status_led_rainbow_cycle::config &config) noexcept
      : config_(config) {}

  std::unique_ptr<status_led_mode::runtime> status_led_rainbow_cycle::apply(led_strip &leds) const noexcept
  {
    return timer_based_runtime::create<status_led_rainbow_cycle_runtime>(
        "status_led_rainbow_cycle_timer",
        status_led_rainbow_cycle_runtime::timer_callback,
        config_.tick,
        &config_,
        leds);
  }

  status_led_solid::status_led_solid(const status_led_solid::config &config) noexcept
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

  std::unique_ptr<status_led_mode::runtime> status_led_solid::apply(led_strip &leds) const noexcept
  {
    set_pixel(leds, config_.led, config_.color);
    leds.refresh();
    // Solid state does not require a runtime, so we return nullptr
    return nullptr;
  }

} // namespace cjf
