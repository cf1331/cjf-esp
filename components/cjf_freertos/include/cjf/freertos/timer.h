#ifndef E8F3A2B1_4D5C_6E7F_8A9B_0C1D2E3F4A5B
#define E8F3A2B1_4D5C_6E7F_8A9B_0C1D2E3F4A5B

#include <chrono>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <optional>

namespace cjf::freertos
{

  /**
   * @brief Statically-allocated RAII wrapper for FreeRTOS software timers
   *
   * Creates timers using xTimerCreateStatic() with embedded storage.
   * Supports both one-shot and auto-reload (periodic) modes.
   *
   * Use for delayed or periodic execution of code from timer daemon task.
   *
   * @note Non-movable because the timer ID points to the embedded callback_context.
   *       Use create() which returns std::optional and relies on guaranteed copy elision.
   */
  class timer
  {
  public:
    /**
     * @brief Timer callback function type
     *
     * The callback receives a pointer to user-provided context data.
     */
    using callback_t = void (*)(void *context);

    /**
     * @brief Create a new timer with static allocation
     *
     * @param name Timer name for debugging
     * @param period Initial timer period in ticks
     * @param auto_reload If true, timer restarts automatically after expiring
     * @param callback Function to call when timer expires
     * @param context User context pointer passed to callback
     * @return std::optional<timer> Timer on success, std::nullopt on failure
     *
     * @note Uses guaranteed copy elision (C++17+) to construct directly in caller's storage
     *
     * Example:
     * @code
     * auto my_timer = cjf::freertos::timer::create("my_timer", pdMS_TO_TICKS(100), true, callback, this);
     * if (my_timer) {
     *     my_timer->start();
     * }
     * @endcode
     */
    [[nodiscard]] static std::optional<timer> create(
        const char *name,
        TickType_t period,
        bool auto_reload,
        callback_t callback,
        void *context = nullptr) noexcept;

    /**
     * @brief Start the timer
     *
     * @param timeout Maximum time to wait for timer command to be processed
     * @return esp_err_t ESP_OK if started successfully
     */
    esp_err_t start(TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Stop the timer
     *
     * @param timeout Maximum time to wait for timer command to be processed
     * @return esp_err_t ESP_OK if stopped successfully
     */
    esp_err_t stop(TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Reset the timer (restart countdown from beginning)
     *
     * @param timeout Maximum time to wait for timer command to be processed
     * @return esp_err_t ESP_OK if reset successfully
     *
     * @note Also starts the timer if not already running
     */
    esp_err_t reset(TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Change the timer period
     *
     * @param new_period New period in ticks
     * @param timeout Maximum time to wait for timer command to be processed
     * @return esp_err_t ESP_OK if period changed successfully
     *
     * @note Also starts the timer if not already running
     */
    esp_err_t set_period(TickType_t new_period, TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Change the timer period (convenience overload)
     *
     * @param new_period New period in milliseconds
     * @param timeout Maximum time to wait for timer command to be processed
     * @return esp_err_t ESP_OK if period changed successfully
     */
    esp_err_t set_period(std::chrono::milliseconds new_period, TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Check if timer is active (running)
     *
     * @return bool True if timer is currently running
     */
    [[nodiscard]] bool is_active() const noexcept;

    /**
     * @brief Get the current timer period
     *
     * @return TickType_t Current period in ticks
     */
    [[nodiscard]] TickType_t get_period() const noexcept;

    /**
     * @brief Update the context pointer passed to callback
     *
     * @param context New context pointer
     */
    void set_context(void *context) noexcept;

    /**
     * @brief Get the FreeRTOS timer handle
     * @return TimerHandle_t Handle to the underlying FreeRTOS timer
     */
    [[nodiscard]] TimerHandle_t handle() const noexcept { return handle_; }

    /**
     * @brief Check if timer is valid (has been created)
     * @return bool True if timer handle is non-null
     */
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

    // Non-copyable, non-movable (timer ID points to embedded callback_context)
    timer(const timer &) = delete;
    timer &operator=(const timer &) = delete;
    timer(timer &&) = delete;
    timer &operator=(timer &&) = delete;

    /**
     * @brief Destroy timer, stopping and deleting the FreeRTOS timer if valid
     */
    ~timer() noexcept;

    /**
     * @brief Constructor for std::optional construction
     * @param name Timer name for debugging
     * @param period Initial timer period in ticks
     * @param auto_reload If true, timer restarts automatically after expiring
     * @param callback Function to call when timer expires
     * @param context User context pointer passed to callback
     *
     * @note Public for std::optional construction with guaranteed copy elision.
     *       Use create() factory and check the returned optional for validity.
     */
    timer(const char *name, TickType_t period, bool auto_reload, callback_t callback, void *context) noexcept;

  private:
    struct callback_context
    {
      callback_t callback;
      void *user_context;
    };

    /**
     * @brief Internal callback that dispatches to user callback
     */
    static void timer_callback(TimerHandle_t xTimer);

    TimerHandle_t handle_ = nullptr;        ///< FreeRTOS timer handle
    StaticTimer_t timer_buffer_{};          ///< Timer control block
    callback_context callback_context_{};   ///< Callback and user context
  };

} // namespace cjf::freertos

#endif /* E8F3A2B1_4D5C_6E7F_8A9B_0C1D2E3F4A5B */
