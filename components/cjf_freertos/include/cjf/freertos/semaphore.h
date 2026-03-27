#ifndef A75E4E88_5352_49A3_9992_34FCBBA4B30C
#define A75E4E88_5352_49A3_9992_34FCBBA4B30C

#include <chrono>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace cjf::freertos
{

  /**
   * @brief Statically-allocated RAII wrapper for FreeRTOS binary semaphores
   *
   * Creates binary semaphores using xSemaphoreCreateBinaryStatic() with embedded
   * storage. Binary semaphores have only two states: available (1) or taken (0).
   *
   * Use for task synchronization and signaling between tasks/ISRs.
   *
   * @note Non-movable because the FreeRTOS handle references the embedded buffer.
   */
  class binary_semaphore
  {
  public:
    /**
     * @brief Construct a binary semaphore with static allocation
     *
     * @param initial_count If true, semaphore starts available; if false, starts taken
     */
    explicit binary_semaphore(bool initial_count = false) noexcept;

    uint32_t count() const noexcept;

    /**
     * @brief Take (acquire) the semaphore
     *
     * @param timeout Maximum time to wait (0 = no wait, portMAX_DELAY = forever)
     * @return esp_err_t ESP_OK if taken, ESP_ERR_TIMEOUT if unavailable
     */
    esp_err_t take(TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Take (acquire) the semaphore (convenience overload)
     *
     * @param timeout Maximum time to wait
     * @return esp_err_t ESP_OK if taken, ESP_ERR_TIMEOUT if unavailable
     */
    esp_err_t take(std::chrono::milliseconds timeout) noexcept;

    /**
     * @brief Give (release) the semaphore
     *
     * @return esp_err_t ESP_OK if given successfully
     */
    esp_err_t give() noexcept;

    /**
     * @brief Give the semaphore from an ISR
     *
     * @param higher_priority_task_woken Set to true if giving unblocks higher priority task
     * @return esp_err_t ESP_OK if given successfully
     */
    esp_err_t give_from_isr(bool &higher_priority_task_woken) noexcept;

    /**
     * @brief Get the FreeRTOS semaphore handle
     * @return SemaphoreHandle_t Handle to the underlying FreeRTOS semaphore
     */
    [[nodiscard]] SemaphoreHandle_t handle() const noexcept { return handle_; }

    /**
     * @brief Check if semaphore is valid (has been created)
     * @return bool True if semaphore handle is non-null
     */
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

    // Non-copyable, non-movable (handle references embedded buffer)
    binary_semaphore(const binary_semaphore &) = delete;
    binary_semaphore &operator=(const binary_semaphore &) = delete;
    binary_semaphore(binary_semaphore &&) = delete;
    binary_semaphore &operator=(binary_semaphore &&) = delete;

    /**
     * @brief Destroy semaphore, deleting the FreeRTOS semaphore if valid
     */
    ~binary_semaphore() noexcept;

  private:
    /**
     * @brief Convert std::chrono::milliseconds to FreeRTOS ticks
     * @param ms Milliseconds duration
     * @return TickType_t Equivalent ticks
     */
    static TickType_t to_ticks(std::chrono::milliseconds ms) noexcept;

    StaticSemaphore_t semaphore_buffer_{}; ///< Semaphore control block
    SemaphoreHandle_t handle_ = nullptr;   ///< FreeRTOS semaphore handle
  };

  /**
   * @brief Statically-allocated RAII wrapper for FreeRTOS mutexes
   *
   * Creates mutexes using xSemaphoreCreateMutexStatic() with embedded storage.
   * Mutexes support priority inheritance to prevent priority inversion.
   *
   * Use for protecting shared resources between tasks (not ISRs).
   *
   * @note Non-movable because the FreeRTOS handle points to the embedded buffer.
   */
  class mutex
  {
  public:
    /**
     * @brief Construct a mutex
     *
     * Creates the mutex using xSemaphoreCreateMutexStatic with embedded storage.
     *
     * @note Mutex starts in unlocked state
     */
    mutex() noexcept;

    /**
     * @brief Lock (acquire) the mutex
     *
     * @param timeout Maximum time to wait (0 = no wait, portMAX_DELAY = forever)
     * @return esp_err_t ESP_OK if locked, ESP_ERR_TIMEOUT if unavailable
     */
    esp_err_t lock(TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Lock (acquire) the mutex (convenience overload)
     *
     * @param timeout Maximum time to wait
     * @return esp_err_t ESP_OK if locked, ESP_ERR_TIMEOUT if unavailable
     */
    esp_err_t lock(std::chrono::milliseconds timeout) noexcept;

    /**
     * @brief Unlock (release) the mutex
     *
     * @return esp_err_t ESP_OK if unlocked successfully
     *
     * @note Must be called from the same task that locked it
     */
    esp_err_t unlock() noexcept;

    /**
     * @brief Get the FreeRTOS mutex handle
     * @return SemaphoreHandle_t Handle to the underlying FreeRTOS mutex
     */
    [[nodiscard]] SemaphoreHandle_t handle() const noexcept { return handle_; }

    /**
     * @brief Check if mutex is valid (has been created)
     * @return bool True if mutex handle is non-null
     */
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

    // Non-copyable, non-movable (handle points to embedded buffer)
    mutex(const mutex &) = delete;
    mutex &operator=(const mutex &) = delete;
    mutex(mutex &&) = delete;
    mutex &operator=(mutex &&) = delete;

    /**
     * @brief Destroy mutex, deleting the FreeRTOS mutex if valid
     */
    ~mutex() noexcept;

  private:
    /**
     * @brief Convert std::chrono::milliseconds to FreeRTOS ticks
     * @param ms Milliseconds duration
     * @return TickType_t Equivalent ticks
     */
    static TickType_t to_ticks(std::chrono::milliseconds ms) noexcept;

    StaticSemaphore_t mutex_buffer_{};   ///< Mutex control block (must be first!)
    SemaphoreHandle_t handle_ = nullptr; ///< FreeRTOS mutex handle
  };

  /**
   * @brief RAII lock guard for mutex
   *
   * Automatically locks mutex on construction and unlocks on destruction.
   */
  class lock_guard
  {
  public:
    /**
     * @brief Construct lock guard and lock the mutex
     *
     * @param m Mutex to lock
     * @param timeout Maximum time to wait for lock
     */
    explicit lock_guard(mutex &m, TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Check if lock was successfully acquired
     * @return bool True if mutex is locked
     */
    [[nodiscard]] bool locked() const noexcept { return locked_; }

    /**
     * @brief Check if lock was successfully acquired
     * @return bool True if mutex is locked
     */
    [[nodiscard]] explicit operator bool() const noexcept { return locked(); }

    // Non-copyable, non-movable
    lock_guard(const lock_guard &) = delete;
    lock_guard &operator=(const lock_guard &) = delete;
    lock_guard(lock_guard &&) = delete;
    lock_guard &operator=(lock_guard &&) = delete;

    /**
     * @brief Destroy lock guard and unlock the mutex if locked
     */
    ~lock_guard() noexcept;

  private:
    mutex &mutex_;
    bool locked_;
  };

} // namespace cjf::freertos

#endif /* A75E4E88_5352_49A3_9992_34FCBBA4B30C */
