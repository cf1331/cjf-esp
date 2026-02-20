#include "cjf/timeout.h"
#include <freertos/FreeRTOS.h>

namespace cjf
{

  timeout::timeout(TickType_t ticks)
      : start_ticks(xTaskGetTickCount()),
        timeout_ticks(ticks)
  {
  }

  timeout timeout::from_ms(int32_t ms) noexcept
  {
    return timeout(pdMS_TO_TICKS(ms));
  }

  TickType_t timeout::duration() const noexcept
  {
    return timeout_ticks;
  }

  int32_t timeout::duration_ms() const noexcept
  {
    return (timeout_ticks == portMAX_DELAY) ? -1 : pdTICKS_TO_MS(timeout_ticks);
  }

  TickType_t timeout::elapsed() const noexcept
  {
    TickType_t now = xTaskGetTickCount();
    return elapsed(now);
  }

  TickType_t timeout::elapsed(TickType_t now) const noexcept
  {
    return now - start_ticks;
  }

  int32_t timeout::elapsed_ms() const noexcept
  {
    TickType_t now = xTaskGetTickCount();
    return elapsed_ms(now);
  }

  int32_t timeout::elapsed_ms(TickType_t now) const noexcept
  {
    TickType_t el = elapsed(now);
    return (el == portMAX_DELAY) ? -1 : pdTICKS_TO_MS(el);
  }

  bool timeout::has_expired() const noexcept
  {
    TickType_t now = xTaskGetTickCount();
    return has_expired(now);
  }

  bool timeout::has_expired(TickType_t now) const noexcept
  {
    return remaining(now) == 0;
  }

  TickType_t timeout::remaining() const noexcept
  {
    TickType_t now = xTaskGetTickCount();
    return remaining(now);
  }

  TickType_t timeout::remaining(TickType_t now) const noexcept
  {
    if (timeout_ticks == portMAX_DELAY)
    {
      return portMAX_DELAY; // Infinite timeout
    }
    TickType_t elapsed_ticks = elapsed(now);
    return (elapsed_ticks < timeout_ticks) ? (timeout_ticks - elapsed_ticks) : 0;
  }

  int32_t timeout::remaining_ms() const noexcept
  {
    TickType_t now = xTaskGetTickCount();
    return remaining_ms(now);
  }

  int32_t timeout::remaining_ms(TickType_t now) const noexcept
  {
    TickType_t rem = remaining(now);
    return (rem == portMAX_DELAY) ? -1 : pdTICKS_TO_MS(rem);
  }

  timeout &timeout::operator=(TickType_t ticks)
  {
    timeout_ticks = ticks;
    return *this;
  }

} // namespace cjf
