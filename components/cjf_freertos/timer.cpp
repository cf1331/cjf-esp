#include "cjf/freertos/timer.h"
#include <optional>

namespace cjf::freertos
{

  std::optional<timer> timer::create(
      const char *name,
      TickType_t period,
      bool auto_reload,
      callback_t callback,
      void *context) noexcept
  {
    // Use std::in_place to construct directly in the optional's storage
    // Guaranteed copy elision ensures no move is required
    return std::optional<timer>(std::in_place, name, period, auto_reload, callback, context);
  }

  timer::timer(const char *name, TickType_t period, bool auto_reload, callback_t callback, void *context) noexcept
      : handle_(nullptr), timer_buffer_{}, callback_context_{callback, context}
  {
    // Create timer with callback_context_ already at its final address
    handle_ = xTimerCreateStatic(
        name,
        period,
        auto_reload ? pdTRUE : pdFALSE,
        &callback_context_,
        timer_callback,
        &timer_buffer_);
  }

  timer::~timer() noexcept
  {
    if (handle_)
    {
      xTimerStop(handle_, portMAX_DELAY);
      xTimerDelete(handle_, portMAX_DELAY);
      handle_ = nullptr;
    }
  }

  void timer::timer_callback(TimerHandle_t xTimer)
  {
    auto *ctx = static_cast<callback_context *>(pvTimerGetTimerID(xTimer));
    if (ctx && ctx->callback)
    {
      ctx->callback(ctx->user_context);
    }
  }

  esp_err_t timer::start(TickType_t timeout) noexcept
  {
    if (!handle_)
      return ESP_ERR_INVALID_STATE;
    return xTimerStart(handle_, timeout) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  esp_err_t timer::stop(TickType_t timeout) noexcept
  {
    if (!handle_)
      return ESP_ERR_INVALID_STATE;
    return xTimerStop(handle_, timeout) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  esp_err_t timer::reset(TickType_t timeout) noexcept
  {
    if (!handle_)
      return ESP_ERR_INVALID_STATE;
    return xTimerReset(handle_, timeout) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  esp_err_t timer::set_period(TickType_t new_period, TickType_t timeout) noexcept
  {
    if (!handle_)
      return ESP_ERR_INVALID_STATE;
    return xTimerChangePeriod(handle_, new_period, timeout) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  esp_err_t timer::set_period(std::chrono::milliseconds new_period, TickType_t timeout) noexcept
  {
    return set_period(pdMS_TO_TICKS(new_period.count()), timeout);
  }

  bool timer::is_active() const noexcept
  {
    return handle_ && xTimerIsTimerActive(handle_) != pdFALSE;
  }

  TickType_t timer::get_period() const noexcept
  {
    return handle_ ? xTimerGetPeriod(handle_) : 0;
  }

  void timer::set_context(void *context) noexcept
  {
    callback_context_.user_context = context;
  }

} // namespace cjf::freertos
