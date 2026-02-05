#ifndef CF4C6730_B1BB_4E71_A357_7E303F09A4EA
#define CF4C6730_B1BB_4E71_A357_7E303F09A4EA

#include <chrono>
#include <esp_err.h>
#include <expected>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

namespace cjf::freertos
{

  /**
   * @brief Statically-allocated RAII wrapper for FreeRTOS event groups
   *
   * Creates event groups using xEventGroupCreateStatic() with embedded storage.
   * Event groups provide a set of binary event flags (bits) that tasks can set,
   * clear, and wait for. Each event group has 24 usable bits (bits 0-23).
   *
   * Use for synchronizing multiple tasks and signaling multiple events.
   */
  class event_group
  {
  public:
    /**
     * @brief Create a new event group with static allocation
     *
     * @return std::expected<event_group, esp_err_t> Event group on success
     *
     * @note All bits start cleared (0)
     */
    [[nodiscard]] static std::expected<event_group, esp_err_t> create() noexcept;

    /**
     * @brief Set (raise) one or more event bits
     *
     * @param bits Bitmask of bits to set
     * @return EventBits_t The value of the event group before the bits were set
     */
    EventBits_t set(EventBits_t bits) noexcept;

    /**
     * @brief Set (raise) one or more event bits from an ISR
     *
     * @param bits Bitmask of bits to set
     * @param higher_priority_task_woken Set to true if setting bits unblocks higher priority task
     * @return esp_err_t ESP_OK if successful
     */
    esp_err_t set_from_isr(EventBits_t bits, bool &higher_priority_task_woken) noexcept;

    /**
     * @brief Clear (lower) one or more event bits
     *
     * @param bits Bitmask of bits to clear
     * @return EventBits_t The value of the event group before the bits were cleared
     */
    EventBits_t clear(EventBits_t bits) noexcept;

    /**
     * @brief Clear (lower) one or more event bits from an ISR
     *
     * @param bits Bitmask of bits to clear
     * @return esp_err_t ESP_OK if successful
     */
    esp_err_t clear_from_isr(EventBits_t bits) noexcept;

    /**
     * @brief Wait for one or more event bits to be set
     *
     * @param bits Bitmask of bits to wait for
     * @param clear_on_exit If true, clear matched bits before returning
     * @param wait_for_all If true, wait for ALL bits; if false, wait for ANY bit
     * @param timeout Maximum time to wait (0 = no wait, portMAX_DELAY = forever)
     * @return std::expected<EventBits_t, esp_err_t> Bits that were set, or timeout error
     *
     * @note Returns the value of the event group at the time the wait condition was met
     */
    [[nodiscard]] std::expected<EventBits_t, esp_err_t> wait(
        EventBits_t bits,
        bool clear_on_exit = false,
        bool wait_for_all = false,
        TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Wait for one or more event bits to be set (convenience overload)
     *
     * @param bits Bitmask of bits to wait for
     * @param clear_on_exit If true, clear matched bits before returning
     * @param wait_for_all If true, wait for ALL bits; if false, wait for ANY bit
     * @param timeout Maximum time to wait
     * @return std::expected<EventBits_t, esp_err_t> Bits that were set, or timeout error
     */
    [[nodiscard]] std::expected<EventBits_t, esp_err_t> wait(
        EventBits_t bits,
        bool clear_on_exit,
        bool wait_for_all,
        std::chrono::milliseconds timeout) noexcept;

    /**
     * @brief Get the current value of the event group (all bits)
     *
     * @return EventBits_t Current value of event group bits
     */
    [[nodiscard]] EventBits_t get() const noexcept;

    /**
     * @brief Get the current value of the event group from an ISR
     *
     * @return EventBits_t Current value of event group bits
     */
    [[nodiscard]] EventBits_t get_from_isr() const noexcept;

    /**
     * @brief Synchronize with other tasks on a barrier
     *
     * Sets specified bits, then waits for all sync bits to be set by other tasks.
     * When all sync bits are set, clears them and returns.
     *
     * @param bits_to_set Bits this task will set
     * @param bits_to_wait_for All bits (from all tasks) required for sync
     * @param timeout Maximum time to wait for sync
     * @return std::expected<EventBits_t, esp_err_t> Final bit pattern, or timeout error
     */
    [[nodiscard]] std::expected<EventBits_t, esp_err_t> sync(
        EventBits_t bits_to_set,
        EventBits_t bits_to_wait_for,
        TickType_t timeout = portMAX_DELAY) noexcept;

    /**
     * @brief Get the FreeRTOS event group handle
     * @return EventGroupHandle_t Handle to the underlying FreeRTOS event group
     */
    [[nodiscard]] EventGroupHandle_t handle() const noexcept { return handle_; }

    /**
     * @brief Check if event group is valid (has been created)
     * @return bool True if event group handle is non-null
     */
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

    // Move-only semantics
    event_group(const event_group &) = delete;
    event_group &operator=(const event_group &) = delete;
    event_group(event_group &&other) noexcept;
    event_group &operator=(event_group &&other) noexcept;

    /**
     * @brief Destroy event group, deleting the FreeRTOS event group if valid
     */
    ~event_group() noexcept;

  private:
    /**
     * @brief Private constructor for factory pattern
     * @param handle FreeRTOS event group handle
     */
    explicit event_group(EventGroupHandle_t handle) noexcept;

    /**
     * @brief Convert std::chrono::milliseconds to FreeRTOS ticks
     * @param ms Milliseconds duration
     * @return TickType_t Equivalent ticks
     */
    static TickType_t to_ticks(std::chrono::milliseconds ms) noexcept;

    EventGroupHandle_t handle_ = nullptr;   ///< FreeRTOS event group handle
    StaticEventGroup_t event_group_buffer_; ///< Event group control block
  };

} // namespace cjf::freertos

#endif /* CF4C6730_B1BB_4E71_A357_7E303F09A4EA */
