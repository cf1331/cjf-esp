#include "cjf/event.h"
#include <cjf/error_handling.h>

namespace cjf
{
  static const char *TAG = "cjf:event";

  std::expected<event_handler, esp_err_t> event_handler::create(
      esp_event_base_t event_base,
      int32_t event_id,
      esp_event_handler_t handler,
      void *event_handler_arg) noexcept
  {
    RETURN_UNEXPECTED_ON_ERROR(esp_event_handler_register(event_base, event_id, handler, event_handler_arg), TAG);
    return event_handler(event_base, event_id, handler);
  }

  std::expected<event_handler, esp_err_t> event_handler::create_with_managed_arg(
      esp_event_base_t event_base,
      int32_t event_id,
      esp_event_handler_t handler,
      void *event_handler_arg,
      void (*deleter)(void *)) noexcept
  {
    RETURN_UNEXPECTED_ON_ERROR(esp_event_handler_register(event_base, event_id, handler, event_handler_arg), TAG);

    std::unique_ptr<void, void (*)(void *)> managed_arg(event_handler_arg, deleter);
    return event_handler(event_base, event_id, handler, std::move(managed_arg));
  }

  event_handler::event_handler(event_handler &&other) noexcept
      : event_base_(std::exchange(other.event_base_, nullptr)),
        event_id_(std::exchange(other.event_id_, NULL)),
        handler_(std::exchange(other.handler_, nullptr)),
        managed_arg_(std::move(other.managed_arg_)) {}

  event_handler &event_handler::operator=(event_handler &&other) noexcept
  {
    if (this != &other)
    {
      event_base_ = std::exchange(other.event_base_, nullptr);
      event_id_ = std::exchange(other.event_id_, NULL);
      handler_ = std::exchange(other.handler_, nullptr);
      managed_arg_ = std::move(other.managed_arg_);
    }
    return *this;
  }

  event_handler::event_handler(
      esp_event_base_t event_base,
      int32_t event_id,
      esp_event_handler_t handler,
      std::unique_ptr<void, void (*)(void *)> managed_arg) noexcept
      : event_base_(event_base), event_id_(event_id), handler_(handler),
        managed_arg_(std::move(managed_arg)) {}

  event_handler::~event_handler()
  {
    if (handler_)
    {
      ESP_LOGD(TAG, "Unregistering event handler for event base '%s', event ID %d", event_base_, event_id_);
      LOG_IF_ERROR(esp_event_handler_unregister(event_base_, event_id_, handler_), TAG);
    }
    // managed_arg_ will be automatically destroyed, calling its deleter
  }

} // namespace cjf
