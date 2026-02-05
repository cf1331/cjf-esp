#ifndef D2AD53B7_7431_4634_A748_7EFDB1DE9E75
#define D2AD53B7_7431_4634_A748_7EFDB1DE9E75

#include <cjf/freertos/task.h>
#include <cjf/task_controller.h>
#include <esp_err.h>
#include <utility>

namespace cjf::freertos
{

  /**
   * @brief Aggregate of task and task_controller for lifecycle management
   *
   * Combines a statically-allocated task with a task_controller for cooperative
   * start/stop signaling. The controller is wired to the task's handle during
   * construction. Forwarding methods simplify access to controller functionality.
   *
   * Neither copyable nor moveable due to embedded task.
   *
   * @tparam StackSize Size of task stack in words
   */
  template <size_t StackSize>
  struct controllable_task
  {
    /**
     * @brief Construct task with controller
     *
     * @tparam Func Callable type (lambda, function pointer, std::function)
     * @param func Task function - must be invocable as void() and noexcept
     * @param name Task name (max 16 characters, used for debugging)
     * @param priority Task priority (tskIDLE_PRIORITY to configMAX_PRIORITIES-1)
     *
     * @note The task function typically captures the controllable_task and calls
     *       wait_for_start() and loops while should_run()
     * @note Use explicit bool cast to check if creation succeeded
     *
     * Example:
     * @code
     * controllable_task<4096> task_ctrl(
     *     [&task_ctrl]() noexcept {
     *         task_ctrl.wait_for_start();
     *         while (task_ctrl.should_run()) {
     *             // work
     *         }
     *     },
     *     "MyTask", 5
     * );
     * @endcode
     */
    template <typename Func>
      requires std::is_invocable_r_v<void, Func> && std::is_nothrow_invocable_v<Func>
    controllable_task(
        Func &&func,
        const char *name,
        UBaseType_t priority = tskIDLE_PRIORITY) noexcept;

    /**
     * @brief Check if task creation succeeded
     * @return bool True if task handle is non-null (creation succeeded)
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
      return static_cast<bool>(task);
    }

    /**
     * @brief Get error status for use with error handling macros
     * @return esp_err_t ESP_OK if task created successfully, ESP_ERR_NO_MEM otherwise
     */
    [[nodiscard]] explicit operator esp_err_t() const noexcept
    {
      return static_cast<esp_err_t>(task);
    }

    // Controller forwarding methods for simplified API

    /**
     * @brief Start the task's work loop
     *
     * Sends start notification to the task. The task function should call
     * wait_for_start() to receive this notification.
     */
    void start() noexcept { controller.start(); }

    /**
     * @brief Stop the task's work loop
     *
     * Sends stop notification to the task. The task function should call
     * should_run() to check for this notification.
     */
    void stop() noexcept { controller.stop(); }

    /**
     * @brief Wait for start notification (blocks until received)
     * @return bool True if start notification received
     *
     * Task function should call this before entering main work loop.
     */
    bool wait_for_start() noexcept { return controller.wait_for_start(); }

    /**
     * @brief Check if task should continue running (non-blocking)
     * @return bool True if no stop notification received, false if stop requested
     *
     * Task function should call this in work loop condition.
     */
    bool should_run() noexcept { return controller.should_run(); }

    // Neither copyable nor moveable - task is immovable
    controllable_task(const controllable_task &) = delete;
    controllable_task &operator=(const controllable_task &) = delete;
    controllable_task(controllable_task &&) = delete;
    controllable_task &operator=(controllable_task &&) = delete;

    cjf::freertos::task<StackSize> task;
    cjf::task_controller controller;
  };

  // ============================================================================
  // Template Implementation
  // ============================================================================

  template <size_t StackSize>
  template <typename Func>
    requires std::is_invocable_r_v<void, Func> && std::is_nothrow_invocable_v<Func>
  controllable_task<StackSize>::controllable_task(
      Func &&func,
      const char *name,
      UBaseType_t priority) noexcept
      : task{std::forward<Func>(func), name, priority}, controller{task.handle()}
  {
  }

} // namespace cjf::freertos

#endif /* D2AD53B7_7431_4634_A748_7EFDB1DE9E75 */
