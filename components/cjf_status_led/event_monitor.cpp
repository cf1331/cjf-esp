#include "cjf/status_led/event_monitor.h"
#include <cjf/error_handling.h>
#include <cjf/event.h>
#include <esp_log.h>
#include <vector>

namespace cjf
{
  static const char *TAG = "cjf:status_led:event_monitor";

  bool event_key_t::operator<(const event_key_t &rhs) const
  {
    return event_base < rhs.event_base || (event_base == rhs.event_base && event_id < rhs.event_id);
  }

  bool event_key_t::operator==(const event_key_t &rhs) const
  {
    return event_base == rhs.event_base && event_id == rhs.event_id;
  }

  std::expected<status_led_event_monitor, esp_err_t> status_led_event_monitor::create(
      cjf::status_led &status_led,
      const event_map &event_modes)
  {
    auto ctx = std::make_unique<context>(event_modes, status_led);
    ctx->event_handlers.reserve(event_modes.size());
    for (const auto &[key, mode] : event_modes)
    {
      auto handler = cjf::event_handler::create(
          key.event_base,
          key.event_id,
          event_handler_,
          ctx.get());
      RETURN_ON_UNEXPECTED(handler, TAG);
      ctx->event_handlers.emplace_back(std::move(*handler));
    }
    return status_led_event_monitor(std::move(ctx));
  }

  status_led_event_monitor::status_led_event_monitor(std::unique_ptr<context> ctx) noexcept
      : ctx_(std::move(ctx)) {}

  void status_led_event_monitor::event_handler_(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
  {
    auto ctx = reinterpret_cast<context *>(arg);
    auto it = ctx->event_modes.find({event_base, event_id});
    if (it == ctx->event_modes.end())
      return;
    auto mode = it->second.get();
    ctx->status_led.set_mode(mode);
    ctx->last_event = {event_base, event_id};
  }

  void status_led_event_monitor::reapply_last_mode() noexcept
  {
    if (ctx_->last_event.event_base == nullptr)
      return;
    auto it = ctx_->event_modes.find(ctx_->last_event);
    if (it == ctx_->event_modes.end())
      return;
    auto mode = it->second.get();
    ctx_->status_led.set_mode(mode);
  }

} // namespace cjf
