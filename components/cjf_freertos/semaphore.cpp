#include "cjf/freertos/semaphore.h"
#include <cjf/error_handling.h>

namespace cjf::freertos
{

  static constexpr const char *TAG = "cjf::freertos::semaphore";

  // ============================================================================
  // binary_semaphore implementation
  // ============================================================================

  binary_semaphore::binary_semaphore(bool initial_count) noexcept
      : semaphore_buffer_{},
        handle_{xSemaphoreCreateBinaryStatic(&semaphore_buffer_)}
  {
    // If initial_count is true, give the semaphore once to make it available
    if (initial_count && handle_ != nullptr)
    {
      xSemaphoreGive(handle_);
    }
  }

  binary_semaphore::~binary_semaphore() noexcept
  {
    if (handle_ != nullptr)
    {
      vSemaphoreDelete(handle_);
      handle_ = nullptr;
    }
  }

  uint32_t binary_semaphore::count() const noexcept
  {
    if (handle_ == nullptr)
    {
      return 0;
    }
    // uxSemaphoreGetCount returns the count of the semaphore (1 if available, 0 if taken)
    return uxSemaphoreGetCount(handle_);
  }

  esp_err_t binary_semaphore::take(TickType_t timeout) noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xSemaphoreTake(handle_, timeout);
    return (result == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  esp_err_t binary_semaphore::take(std::chrono::milliseconds timeout) noexcept
  {
    return take(to_ticks(timeout));
  }

  esp_err_t binary_semaphore::give() noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xSemaphoreGive(handle_);
    return (result == pdTRUE) ? ESP_OK : ESP_FAIL;
  }

  esp_err_t binary_semaphore::give_from_isr(bool &higher_priority_task_woken) noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xSemaphoreGiveFromISR(handle_, &xHigherPriorityTaskWoken);
    higher_priority_task_woken = (xHigherPriorityTaskWoken == pdTRUE);
    return (result == pdTRUE) ? ESP_OK : ESP_FAIL;
  }

  TickType_t binary_semaphore::to_ticks(std::chrono::milliseconds ms) noexcept
  {
    if (ms.count() < 0)
    {
      return 0;
    }
    return pdMS_TO_TICKS(static_cast<uint32_t>(ms.count()));
  }

  // ============================================================================
  // mutex implementation
  // ============================================================================

  mutex::mutex() noexcept
      : mutex_buffer_{}, handle_{xSemaphoreCreateMutexStatic(&mutex_buffer_)}
  {
    ESP_LOGD(TAG, "Created mutex");
  }

  mutex::~mutex() noexcept
  {
    if (handle_ != nullptr)
    {
      vSemaphoreDelete(handle_);
      handle_ = nullptr;
    }
  }

  esp_err_t mutex::lock(TickType_t timeout) noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xSemaphoreTake(handle_, timeout);
    return (result == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  esp_err_t mutex::lock(std::chrono::milliseconds timeout) noexcept
  {
    return lock(to_ticks(timeout));
  }

  esp_err_t mutex::unlock() noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xSemaphoreGive(handle_);
    return (result == pdTRUE) ? ESP_OK : ESP_FAIL;
  }

  TickType_t mutex::to_ticks(std::chrono::milliseconds ms) noexcept
  {
    if (ms.count() < 0)
    {
      return 0;
    }
    return pdMS_TO_TICKS(static_cast<uint32_t>(ms.count()));
  }

  // ============================================================================
  // lock_guard implementation
  // ============================================================================

  lock_guard::lock_guard(mutex &m, TickType_t timeout) noexcept
      : mutex_{m}, locked_{m.lock(timeout) == ESP_OK}
  {
  }

  lock_guard::~lock_guard() noexcept
  {
    if (locked_)
    {
      mutex_.unlock();
    }
  }

} // namespace cjf::freertos
