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

  /* event_handler_2 ══════════════════════════════════════════════════════════════════════════════ */

  std::expected<event_handler_2, esp_err_t> &
  event_handler_2::emplace_and_register_with_default_loop(
      std::expected<event_handler_2, esp_err_t> &event_handler,
      const esp_event_base_t event_base, const int32_t event_id,
      handler_func handler) noexcept
  {
    event_handler.emplace(std::move(handler));
    auto res = event_handler->register_with_default_loop(event_base, event_id);
    if (res != ESP_OK)
    {
      event_handler = std::unexpected(res);
    }
    return event_handler;
  }

  event_handler_2::event_handler_2(handler_func handler) noexcept
      : handler_(std::move(handler))
  {
  }

  event_handler_2::event_handler_2(event_handler_2 &&other) noexcept
  {
    move_registration_(std::move(other));
  }

  event_handler_2 &event_handler_2::operator=(event_handler_2 &&other) noexcept
  {
    if (this != &other)
    {
      move_registration_(std::move(other));
    }
    return *this;
  }

  event_handler_2::~event_handler_2()
  {
    if (handler_ && event_base_ && event_id_)
    {
      ESP_LOGD(TAG, "Unregistering event handler for event base '%s', event ID %d", event_base_, event_id_);
      LOG_IF_ERROR(esp_event_handler_unregister(event_base_, event_id_, event_dispatcher_), TAG);
    }
  }

  esp_err_t event_handler_2::register_with_default_loop(esp_event_base_t event_base, int32_t event_id) noexcept
  {
    if (!handler_ || event_base_ || event_id_)
    {
      return ESP_ERR_INVALID_STATE;
    }
    RETURN_ON_ERROR(esp_event_handler_register(event_base, event_id, event_dispatcher_, this), TAG);
    event_base_ = event_base;
    event_id_ = event_id;
    ESP_LOGI(TAG, "Registered event handler for event base '%s', event ID %d", event_base_, event_id_);
    return ESP_OK;
  }

  void event_handler_2::event_dispatcher_(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) noexcept
  {
    auto self = static_cast<event_handler_2 *>(arg);
    if (self && self->handler_)
    {
      self->handler_(event_base, event_id, event_data);
    }
  }

  void event_handler_2::move_registration_(event_handler_2 &&other) noexcept
  {
    bool is_already_registered = event_base_ && event_id_;
    bool is_other_already_registered = other.event_base_ && other.event_id_;
    // The ability to move is included for ease of use during the initialisation phase of a program, before
    // the handler has been registered with an event loop.
    //
    // Moving an already registered handler is not recommended because `event_dispatcher_` is registered
    // with the ESP event loop using the address of this object (`this`) as the callback argument. After a
    // move, the new object occupies a different memory address, but the event loop still holds the original
    // (source) address. Until the handler is re-registered, any events delivered during that window will
    // invoke `event_dispatcher_` with a now-stale pointer — which is undefined behaviour and likely to
    // crash the system. Even if the re-registration succeeds, the unregister→re-register gap means events
    // may be silently dropped. However, if it does occur, we attempt to handle it gracefully. Any failure
    // during the move will cause the system to abort().
    if (is_already_registered)
    {
      ESP_LOGW(TAG, "Moving in to an already registered handler for event base '%s', event ID %d. This is probably a bug.", event_base_, event_id_);
    }
    else if (is_other_already_registered)
    {
      ESP_LOGW(TAG, "Moving from an already registered handler for event base '%s', event ID %d. This is probably a bug.", other.event_base_, other.event_id_);
    }

    if (is_already_registered)
    {
      ESP_LOGW(TAG, "Target handler is already registered, attempting to unregister");
      ESP_ERROR_CHECK(esp_event_handler_unregister(event_base_, event_id_, event_dispatcher_));
    }
    if (is_other_already_registered)
    {
      ESP_LOGW(TAG, "Unregistering existing handler");
      ESP_ERROR_CHECK(esp_event_handler_unregister(other.event_base_, other.event_id_, event_dispatcher_));
      ESP_LOGW(TAG, "Registering handler with move target");
      ESP_ERROR_CHECK(esp_event_handler_register(event_base_, event_id_, event_dispatcher_, this));
    }
    handler_ = std::move(other.handler_);
    event_base_ = std::exchange(other.event_base_, nullptr);
    event_id_ = std::exchange(other.event_id_, 0);
  }

} // namespace cjf
