#include "cjf/timeout.h"
#include <freertos/FreeRTOS.h>

namespace cjf
{

  timeout::timeout(TickType_t ticks)
      : start_ticks(xTaskGetTickCount()),
        timeout_ticks(ticks)
  {
  }

  bool timeout::is_expired() const
  {
    return remaining() == 0;
  }

  TickType_t timeout::elapsed() const
  {
    TickType_t now = xTaskGetTickCount();
    return now - start_ticks;
  }

  TickType_t timeout::remaining() const
  {
    TickType_t elapsed_ticks = elapsed();
    return (elapsed_ticks < timeout_ticks) ? (timeout_ticks - elapsed_ticks) : 0;
  }

} // namespace cjf
