#include "cjf/freertos/event_group.h"
#include <cjf/error_handling.h>

namespace cjf::freertos
{
  static constexpr const char *TAG = "cjf::freertos::event_group";

  std::expected<event_group, esp_err_t> event_group::create() noexcept
  {
    event_group eg{nullptr};

    // Create event group with static allocation
    eg.handle_ = xEventGroupCreateStatic(&eg.event_group_buffer_);

    if (eg.handle_ == nullptr)
    {
      ESP_LOGE(TAG, "Failed to create event group");
      return std::unexpected(ESP_FAIL);
    }

    ESP_LOGD(TAG, "Created event group");
    return eg;
  }

  event_group::event_group(EventGroupHandle_t handle) noexcept
      : handle_{handle}
  {
  }

  event_group::event_group(event_group &&other) noexcept
      : handle_{other.handle_}, event_group_buffer_{other.event_group_buffer_}
  {
    other.handle_ = nullptr;
  }

  event_group &event_group::operator=(event_group &&other) noexcept
  {
    if (this != &other)
    {
      if (handle_ != nullptr)
      {
        vEventGroupDelete(handle_);
      }

      handle_ = other.handle_;
      event_group_buffer_ = other.event_group_buffer_;

      other.handle_ = nullptr;
    }
    return *this;
  }

  event_group::~event_group() noexcept
  {
    if (handle_ != nullptr)
    {
      vEventGroupDelete(handle_);
      handle_ = nullptr;
    }
  }

  EventBits_t event_group::set(EventBits_t bits) noexcept
  {
    if (handle_ == nullptr)
    {
      return 0;
    }
    return xEventGroupSetBits(handle_, bits);
  }

  esp_err_t event_group::set_from_isr(EventBits_t bits, bool &higher_priority_task_woken) noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xEventGroupSetBitsFromISR(handle_, bits, &xHigherPriorityTaskWoken);
    higher_priority_task_woken = (xHigherPriorityTaskWoken == pdTRUE);
    return (result == pdPASS) ? ESP_OK : ESP_FAIL;
  }

  EventBits_t event_group::clear(EventBits_t bits) noexcept
  {
    if (handle_ == nullptr)
    {
      return 0;
    }
    return xEventGroupClearBits(handle_, bits);
  }

  esp_err_t event_group::clear_from_isr(EventBits_t bits) noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xEventGroupClearBitsFromISR(handle_, bits);
    return (result == pdPASS) ? ESP_OK : ESP_FAIL;
  }

  std::expected<EventBits_t, esp_err_t> event_group::wait(
      EventBits_t bits,
      bool clear_on_exit,
      bool wait_for_all,
      TickType_t timeout) noexcept
  {
    if (handle_ == nullptr)
    {
      return std::unexpected(ESP_ERR_INVALID_STATE);
    }

    EventBits_t result = xEventGroupWaitBits(
        handle_,
        bits,
        clear_on_exit ? pdTRUE : pdFALSE,
        wait_for_all ? pdTRUE : pdFALSE,
        timeout);

    // Check if timeout occurred
    // If waiting for all bits and not all are set, it's a timeout
    if (wait_for_all && (result & bits) != bits)
    {
      return std::unexpected(ESP_ERR_TIMEOUT);
    }
    // If waiting for any bit and none are set, it's a timeout
    if (!wait_for_all && (result & bits) == 0)
    {
      return std::unexpected(ESP_ERR_TIMEOUT);
    }

    return result;
  }

  std::expected<EventBits_t, esp_err_t> event_group::wait(
      EventBits_t bits,
      bool clear_on_exit,
      bool wait_for_all,
      std::chrono::milliseconds timeout) noexcept
  {
    return wait(bits, clear_on_exit, wait_for_all, to_ticks(timeout));
  }

  EventBits_t event_group::get() const noexcept
  {
    if (handle_ == nullptr)
    {
      return 0;
    }
    return xEventGroupGetBits(handle_);
  }

  EventBits_t event_group::get_from_isr() const noexcept
  {
    if (handle_ == nullptr)
    {
      return 0;
    }
    return xEventGroupGetBitsFromISR(handle_);
  }

  std::expected<EventBits_t, esp_err_t> event_group::sync(
      EventBits_t bits_to_set,
      EventBits_t bits_to_wait_for,
      TickType_t timeout) noexcept
  {
    if (handle_ == nullptr)
    {
      return std::unexpected(ESP_ERR_INVALID_STATE);
    }

    EventBits_t result = xEventGroupSync(
        handle_,
        bits_to_set,
        bits_to_wait_for,
        timeout);

    // Check if timeout occurred (not all sync bits were set)
    if ((result & bits_to_wait_for) != bits_to_wait_for)
    {
      return std::unexpected(ESP_ERR_TIMEOUT);
    }

    return result;
  }

  TickType_t event_group::to_ticks(std::chrono::milliseconds ms) noexcept
  {
    if (ms.count() < 0)
    {
      return 0;
    }
    return pdMS_TO_TICKS(static_cast<uint32_t>(ms.count()));
  }

} // namespace cjf::freertos
