#ifndef C8E1CF1E_FC6B_49C9_B0DF_23D610B7B996
#define C8E1CF1E_FC6B_49C9_B0DF_23D610B7B996

#include <chrono>
#include <cstddef>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <type_traits>

namespace cjf::freertos
{

  /**
   * @brief Statically-allocated RAII wrapper for FreeRTOS queues
   *
   * Creates queues using xQueueCreateStatic() with embedded storage, eliminating
   * heap allocation. Type-safe wrapper ensuring items are copyable and trivially
   * destructible (required for queue storage).
   *
   * Neither copyable nor moveable - storage address is registered with FreeRTOS.
   *
   * @tparam ItemType Type of items stored in queue (must be trivially destructible)
   * @tparam QueueLength Maximum number of items the queue can hold
   */
  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  class queue
  {
  public:
    using item_type = ItemType;
    static constexpr size_t length = QueueLength;

    /**
     * @brief Construct a new queue with static allocation
     *
     * Constructs the queue in-place at its final memory location, ensuring
     * the storage pointer passed to xQueueCreateStatic is stable.
     *
     * @note Use explicit bool cast to check if creation succeeded
     */
    queue() noexcept;

    /**
     * @brief Send an item to the back of the queue
     *
     * @param item Item to send (will be copied into queue)
     * @param timeout Maximum time to wait (0 = no wait, portMAX_DELAY = forever)
     * @return esp_err_t ESP_OK on success, ESP_ERR_TIMEOUT if full
     */
    esp_err_t send(const ItemType &item, TickType_t timeout = 0) noexcept;

    /**
     * @brief Send an item to the back of the queue (convenience overload)
     *
     * @param item Item to send (will be copied into queue)
     * @param timeout Maximum time to wait
     * @return esp_err_t ESP_OK on success, ESP_ERR_TIMEOUT if full
     */
    esp_err_t send(const ItemType &item, std::chrono::milliseconds timeout) noexcept;

    /**
     * @brief Send an item to the front of the queue
     *
     * @param item Item to send (will be copied into queue)
     * @param timeout Maximum time to wait (0 = no wait, portMAX_DELAY = forever)
     * @return esp_err_t ESP_OK on success, ESP_ERR_TIMEOUT if full
     */
    esp_err_t send_to_front(const ItemType &item, TickType_t timeout = 0) noexcept;

    /**
     * @brief Receive an item from the front of the queue
     *
     * @param item Reference to store received item
     * @param timeout Maximum time to wait (0 = no wait, portMAX_DELAY = forever)
     * @return esp_err_t ESP_OK on success, ESP_ERR_TIMEOUT if empty
     */
    esp_err_t receive(ItemType &item, TickType_t timeout = 0) noexcept;

    /**
     * @brief Receive an item from the front of the queue (convenience overload)
     *
     * @param item Reference to store received item
     * @param timeout Maximum time to wait
     * @return esp_err_t ESP_OK on success, ESP_ERR_TIMEOUT if empty
     */
    esp_err_t receive(ItemType &item, std::chrono::milliseconds timeout) noexcept;

    /**
     * @brief Peek at the front item without removing it
     *
     * @param item Reference to store peeked item
     * @param timeout Maximum time to wait for item to be available
     * @return esp_err_t ESP_OK on success, ESP_ERR_TIMEOUT if empty
     */
    esp_err_t peek(ItemType &item, TickType_t timeout = 0) noexcept;

    /**
     * @brief Get number of messages currently in the queue
     * @return size_t Number of items in queue
     */
    [[nodiscard]] size_t size() const noexcept;

    /**
     * @brief Get number of free spaces in the queue
     * @return size_t Number of free slots
     */
    [[nodiscard]] size_t available() const noexcept;

    /**
     * @brief Check if queue is empty
     * @return bool True if queue has no items
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Check if queue is full
     * @return bool True if queue cannot accept more items
     */
    [[nodiscard]] bool full() const noexcept;

    /**
     * @brief Reset the queue to empty state
     * @return esp_err_t ESP_OK on success
     */
    esp_err_t reset() noexcept;

    /**
     * @brief Get the FreeRTOS queue handle
     * @return QueueHandle_t Handle to the underlying FreeRTOS queue
     */
    [[nodiscard]] QueueHandle_t handle() const noexcept { return handle_; }

    /**
     * @brief Check if queue is valid (has been created)
     * @return bool True if queue handle is non-null
     */
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

    /**
     * @brief Get error status for use with error handling macros
     * @return esp_err_t ESP_OK if queue created successfully, ESP_ERR_NO_MEM otherwise
     */
    [[nodiscard]] explicit operator esp_err_t() const noexcept
    {
      return handle_ != nullptr ? ESP_OK : ESP_ERR_NO_MEM;
    }

    // Neither copyable nor moveable - storage pointer is registered with FreeRTOS
    queue(const queue &) = delete;
    queue &operator=(const queue &) = delete;
    queue(queue &&other) = delete;
    queue &operator=(queue &&other) = delete;

    /**
     * @brief Destroy queue, deleting the FreeRTOS queue if valid
     */
    ~queue() noexcept;

  private:
    /**
     * @brief Convert std::chrono::milliseconds to FreeRTOS ticks
     * @param ms Milliseconds duration
     * @return TickType_t Equivalent ticks
     */
    static TickType_t to_ticks(std::chrono::milliseconds ms) noexcept;

    QueueHandle_t handle_ = nullptr;                  ///< FreeRTOS queue handle
    StaticQueue_t queue_buffer_;                      ///< Queue control block
    uint8_t storage_[QueueLength * sizeof(ItemType)]; ///< Statically allocated storage
  };

  // ============================================================================
  // Template Implementation
  // ============================================================================

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  queue<ItemType, QueueLength>::queue() noexcept
      : handle_{nullptr}
  {
    static constexpr const char *TAG = "cjf::freertos::queue";

    // Call xQueueCreateStatic with storage pointers
    // Safe: object is already at its final memory location
    handle_ = xQueueCreateStatic(
        QueueLength,
        sizeof(ItemType),
        storage_,
        &queue_buffer_);

    if (handle_ == nullptr)
    {
      ESP_LOGE(TAG, "Failed to create queue (length=%zu, item_size=%zu) - errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY",
               QueueLength, sizeof(ItemType));
    }
    else
    {
      ESP_LOGD(TAG, "Created queue (length=%zu, item_size=%zu)",
               QueueLength, sizeof(ItemType));
    }
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  queue<ItemType, QueueLength>::~queue() noexcept
  {
    if (handle_ != nullptr)
    {
      vQueueDelete(handle_);
      handle_ = nullptr;
    }
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  esp_err_t queue<ItemType, QueueLength>::send(const ItemType &item, TickType_t timeout) noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xQueueSend(handle_, &item, timeout);
    return (result == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  esp_err_t queue<ItemType, QueueLength>::send(const ItemType &item, std::chrono::milliseconds timeout) noexcept
  {
    return send(item, to_ticks(timeout));
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  esp_err_t queue<ItemType, QueueLength>::send_to_front(const ItemType &item, TickType_t timeout) noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xQueueSendToFront(handle_, &item, timeout);
    return (result == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  esp_err_t queue<ItemType, QueueLength>::receive(ItemType &item, TickType_t timeout) noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xQueueReceive(handle_, &item, timeout);
    return (result == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  esp_err_t queue<ItemType, QueueLength>::receive(ItemType &item, std::chrono::milliseconds timeout) noexcept
  {
    return receive(item, to_ticks(timeout));
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  esp_err_t queue<ItemType, QueueLength>::peek(ItemType &item, TickType_t timeout) noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xQueuePeek(handle_, &item, timeout);
    return (result == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  size_t queue<ItemType, QueueLength>::size() const noexcept
  {
    if (handle_ == nullptr)
    {
      return 0;
    }
    return uxQueueMessagesWaiting(handle_);
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  size_t queue<ItemType, QueueLength>::available() const noexcept
  {
    if (handle_ == nullptr)
    {
      return 0;
    }
    return uxQueueSpacesAvailable(handle_);
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  bool queue<ItemType, QueueLength>::empty() const noexcept
  {
    return size() == 0;
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  bool queue<ItemType, QueueLength>::full() const noexcept
  {
    return available() == 0;
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  esp_err_t queue<ItemType, QueueLength>::reset() noexcept
  {
    if (handle_ == nullptr)
    {
      return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xQueueReset(handle_);
    return (result == pdTRUE) ? ESP_OK : ESP_FAIL;
  }

  template <typename ItemType, size_t QueueLength>
    requires std::is_trivially_destructible_v<ItemType>
  TickType_t queue<ItemType, QueueLength>::to_ticks(std::chrono::milliseconds ms) noexcept
  {
    if (ms.count() < 0)
    {
      return 0;
    }
    return pdMS_TO_TICKS(static_cast<uint32_t>(ms.count()));
  }

} // namespace cjf::freertos

#endif /* C8E1CF1E_FC6B_49C9_B0DF_23D610B7B996 */
