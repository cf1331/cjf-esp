#ifndef A1897129_ED96_4571_833F_45BDF2F35BD1
#define A1897129_ED96_4571_833F_45BDF2F35BD1

#include <concepts>
#include <cstddef>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>
#include <utility>

namespace cjf::freertos
{

  /**
   * @brief Statically-allocated RAII wrapper for FreeRTOS tasks
   *
   * Creates tasks using xTaskCreateStatic() with embedded stack storage, eliminating
   * heap allocation. Move-only semantics ensure single ownership. Task function must
   * be noexcept to prevent exceptions in task context.
   *
   * Use explicit bool cast to check if construction succeeded.
   *
   * @tparam StackSize Size of task stack in words (not bytes)
   */
  template <size_t StackSize>
  class task
  {
  public:
    /**
     * @brief Construct a new task with static allocation
     *
     * Constructs the task in-place at its final memory location, ensuring
     * the 'this' pointer passed to xTaskCreateStatic is stable.
     *
     * @tparam Func Callable type (lambda, function pointer, std::function)
     * @param func Task function - must be invocable as void() and noexcept
     * @param name Task name (max 16 characters, used for debugging)
     * @param priority Task priority (tskIDLE_PRIORITY to configMAX_PRIORITIES-1)
     *
     * @note Func must satisfy is_nothrow_invocable to prevent exceptions in task context
     * @note Use explicit bool cast to check if creation succeeded
     * @note Only failure mode is errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY
     */
    template <typename Func>
      requires std::is_invocable_r_v<void, Func> && std::is_nothrow_invocable_v<Func>
    task(
        Func &&func,
        const char *name,
        UBaseType_t priority = tskIDLE_PRIORITY) noexcept;

    /**
     * @brief Get the FreeRTOS task handle
     * @return TaskHandle_t Handle to the underlying FreeRTOS task
     */
    [[nodiscard]] TaskHandle_t handle() const noexcept { return handle_; }

    /**
     * @brief Check if task creation succeeded
     * @return bool True if task handle is non-null (creation succeeded)
     */
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

    /**
     * @brief Get error status for use with error handling macros
     * @return esp_err_t ESP_OK if task created successfully, ESP_ERR_NO_MEM otherwise
     *
     * @note Enables use with RETURN_ON_ERROR and similar macros:
     * @code
     * task<4096> my_task(...);
     * RETURN_ON_ERROR(my_task, TAG);
     * @endcode
     */
    [[nodiscard]] explicit operator esp_err_t() const noexcept
    {
      return handle_ != nullptr ? ESP_OK : ESP_ERR_NO_MEM;
    }

    // Neither copyable nor moveable - 'this' pointer is registered with FreeRTOS
    task(const task &) = delete;
    task &operator=(const task &) = delete;
    task(task &&other) = delete;
    task &operator=(task &&other) = delete;

    /**
     * @brief Destroy task, deleting the FreeRTOS task if valid
     */
    ~task() noexcept;

  private:
    /**
     * @brief Trampoline function that FreeRTOS calls
     *
     * Extracts the std::function from the parameter and invokes it.
     * When the function returns, deletes the task.
     *
     * @param param Pointer to the task object
     */
    static void task_wrapper(void *param) noexcept;

    std::function<void()> func_;    ///< User task function
    TaskHandle_t handle_ = nullptr; ///< FreeRTOS task handle
    StaticTask_t task_buffer_;      ///< Task control block
    StackType_t stack_[StackSize];  ///< Statically allocated stack
  };

  // ============================================================================
  // Template Implementation
  // ============================================================================

  template <size_t StackSize>
  template <typename Func>
    requires std::is_invocable_r_v<void, Func> && std::is_nothrow_invocable_v<Func>
  task<StackSize>::task(
      Func &&func,
      const char *name,
      UBaseType_t priority) noexcept
      : func_{std::forward<Func>(func)}, handle_{nullptr}
  {
    static constexpr const char *TAG = "cjf::freertos::task";

    // Call xTaskCreateStatic with 'this' pointer
    // Safe: object is already at its final memory location
    handle_ = xTaskCreateStatic(
        &task::task_wrapper,
        name,
        StackSize,
        this, // Stable pointer to this object
        priority,
        stack_,
        &task_buffer_);

    if (handle_ == nullptr)
    {
      ESP_LOGE(TAG, "Failed to create task '%s' - errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY", name);
    }
    else
    {
      ESP_LOGD(TAG, "Created task '%s' (priority=%u, stack=%zu)", name, priority, StackSize);
    }
  }

  template <size_t StackSize>
  task<StackSize>::~task() noexcept
  {
    if (handle_ != nullptr)
    {
      vTaskDelete(handle_);
      handle_ = nullptr;
    }
  }

  template <size_t StackSize>
  void task<StackSize>::task_wrapper(void *param) noexcept
  {
    // Extract task object pointer
    auto *task_obj = static_cast<task *>(param);

    // Invoke user function
    if (task_obj && task_obj->func_)
    {
      task_obj->func_();
    }

    // Task function returned, delete the task
    vTaskDelete(nullptr); // Delete self
  }

} // namespace cjf::freertos

#endif /* A1897129_ED96_4571_833F_45BDF2F35BD1 */
