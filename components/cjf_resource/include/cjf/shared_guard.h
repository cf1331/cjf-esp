#ifndef C67B5364_3482_4E5B_8C0E_93FB5D9857D1
#define C67B5364_3482_4E5B_8C0E_93FB5D9857D1

/**
 * @file shared_guard.h
 * @brief RAII guard for preventing resources from entering harmful states
 *
 * Provides cooperative protection for resources that must remain active while
 * any component needs them. Multiple guards can coexist, all keeping the resource
 * active. Only when ALL guards are destroyed does the resource transition to its
 * inactive state via the deactivator.
 *
 * Common use case: Power rails that multiple peripherals share. Each peripheral
 * acquires a guard when it needs power. The rail only powers off when the last
 * peripheral releases its guard.
 *
 * Zero heap allocation - the control block lives in the resource itself.
 * Thread-safe via FreeRTOS critical sections, safe for use from ISRs.
 */

#include <atomic>
#include <cstddef>
#include <utility>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/portmacro.h>

namespace cjf
{
  static const char *TG = "cjf:resource";
  /**
   * @brief RAII guard that keeps a resource active while in scope
   *
   * Multiple guards can protect the same resource simultaneously. The resource
   * remains active as long as at least one guard exists. When the last guard
   * is destroyed (refcount reaches 0), the deactivator is invoked to transition
   * the resource to its inactive state.
   *
   * Thread-safe via FreeRTOS critical sections, usable from ISRs.
   *
   * @tparam Deactivator Functor with `operator()()` taking no arguments.
   *                     Called when the last guard is destroyed.
   *
   * Example usage:
   * ```cpp
   * class power_rail {
   * public:
   *   struct power_off_deactivator {
   *     void operator()() const noexcept {
   *       // Power off the rail here
   *     }
   *   };
   *   using guard_type = cjf::shared_guard<power_off_deactivator>;
   *
   *   guard_type::control_block guard_ctrl_;
   *
   *   guard_type power_on() {
   *     if (guard_type::needs_activation(guard_ctrl_)) {
   *       // Power on the rail here
   *     }
   *     return guard_type(guard_ctrl_);
   *   }
   * };
   * ```
   */
  template <typename Deactivator>
  class shared_guard
  {
  public:
    struct control_block
    {
      std::atomic<uint32_t> refcount{0};
      portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
      Deactivator deactivator;

      // Constructor with optional deactivator parameter
      explicit control_block(Deactivator deact = Deactivator{}) noexcept
          : refcount{0}, spinlock(portMUX_INITIALIZER_UNLOCKED), deactivator(std::move(deact)) {}

      // Control blocks must have stable addresses since shared_guard holds pointers to them.
      // Therefore, they cannot be moved or copied.
      control_block(const control_block &) = delete;
      control_block &operator=(const control_block &) = delete;
      control_block(control_block &&) = delete;
      control_block &operator=(control_block &&) = delete;
    };

    /**
     * @brief Check if the resource needs activation
     *
     * Returns true if no guards currently exist (refcount is 0), indicating
     * that the resource should be activated before creating the first guard.
     * Thread-safe.
     *
     * @param ctrl Control block to check
     * @return `true` if resource needs activation, otherwise `false`
     *
     * Example:
     * ```
     * if (guard_type::needs_activation(guard_ctrl_)) {
     *   // Activate resource here
     * }
     * return guard_type(guard_ctrl_);
     * ```
     */
    static bool needs_activation(const control_block &ctrl) noexcept
    {
      return ctrl.refcount.load(std::memory_order_relaxed) == 0;

    }

    /**
     * @brief Construct a guard protecting the resource
     *
     * Increments the reference count. The resource will remain active as long
     * as this guard (or any other guard for the same resource) exists.
     * Thread-safe.
     *
     * @param ctrl Control block from the resource (must outlive all guards)
     */
    explicit shared_guard(control_block &ctrl) noexcept
        : ctrl_(&ctrl)
    {
      taskENTER_CRITICAL(&ctrl_->spinlock);
      ctrl_->refcount.fetch_add(1, std::memory_order_relaxed);
      taskEXIT_CRITICAL(&ctrl_->spinlock);
      ESP_LOGD(TG, "New shared guard %p = %lu",&ctrl_,refcount());
    }

    /**
     * @brief Copy constructor - creates another guard for the same resource
     *
     * Creates a new guard protecting the same resource. Both this guard and
     * the original will now keep the resource active. The resource will only
     * deactivate when ALL guards (including copies) are destroyed.
     *
     * Increments the reference count. Thread-safe.
     *
     * Use case: Multiple components need the resource active concurrently.
     * Each can hold its own guard copy.
     *
     * @param other Guard to copy from
     */
    shared_guard(const shared_guard &other) noexcept
        : ctrl_(other.ctrl_)
    {
      if (ctrl_)
      {
        taskENTER_CRITICAL(&ctrl_->spinlock);
        ctrl_->refcount.fetch_add(1, std::memory_order_relaxed);
        taskEXIT_CRITICAL(&ctrl_->spinlock);
        ESP_LOGD(TG, "Copied shared guard %p = %lu",&ctrl_,refcount());
      }
    }

    /**
     * @brief Copy assignment - switches to protecting a different resource
     *
     * Stops protecting the current resource (if any). If this was the last guard
     * for the old resource, its deactivator is invoked. Then begins protecting
     * the same resource as 'other', incrementing its reference count.
     *
     * Result: This guard now protects the same resource as 'other'.
     * Thread-safe.
     *
     * @param other Guard to copy from
     * @return Reference to this guard
     */
    shared_guard &operator=(const shared_guard &other) noexcept
    {
      if (this != &other)
      {
        release();
        ctrl_ = other.ctrl_;
        if (ctrl_)
        {
          taskENTER_CRITICAL(&ctrl_->spinlock);
          ctrl_->refcount.fetch_add(1, std::memory_order_relaxed);
          taskEXIT_CRITICAL(&ctrl_->spinlock);
          ESP_LOGD(TG, "Copied shared guard %p = %lu",&ctrl_,refcount());
        }
      }
      return *this;
    }

    /**
     * @brief Move constructor - transfers protection without refcount change
     *
     * Optimization: This guard takes over protecting the resource from 'other'
     * without changing the reference count (still represents one guard).
     * The source guard is left in a released state and no longer protects anything.
     *
     * Use case: Returning guards from functions efficiently.
     *
     * @param other Guard to move from (left in released state)
     */
    shared_guard(shared_guard &&other) noexcept
        : ctrl_(std::exchange(other.ctrl_, nullptr))
    {
    }

    /**
     * @brief Move assignment - switches to protecting a different resource (optimized)
     *
     * Stops protecting the current resource (if any). If this was the last guard
     * for the old resource, its deactivator is invoked. Then takes over protecting
     * the resource from 'other' without changing its refcount (efficient transfer).
     * The source guard is left in a released state.
     *
     * @param other Guard to move from (left in released state)
     * @return Reference to this guard
     */
    shared_guard &operator=(shared_guard &&other) noexcept
    {
      if (this != &other)
      {
        release();
        ctrl_ = std::exchange(other.ctrl_, nullptr);
      }
      return *this;
    }

    /**
     * @brief Destructor - releases shared ownership
     *
     * Decrements the reference count. If this was the last guard protecting
     * the resource (refcount 1→0), invokes the deactivator to transition the
     * resource to its inactive/off state. Thread-safe.
     */
    ~shared_guard() noexcept
    {
      release();
    }

    /**
     * @brief Check if this guard is actively protecting a resource
     *
     * @return `true` if this guard is valid and protecting a resource,
     *         `false` if it has been moved-from or reset
     */
    explicit operator bool() const noexcept
    {
      return ctrl_ != nullptr;
    }

    /**
     * @brief Get the current number of guards protecting the resource
     *
     * Returns how many guards currently exist for the resource this guard
     * is protecting. Useful for debugging to verify shared ownership.
     *
     * @return Current reference count (number of active guards), or 0 if
     *         this guard has been released/moved-from
     *
     * @note The value may change immediately after reading if other threads
     *       are creating/destroying guards concurrently. Use only for
     *       debugging/logging, not for program logic.
     */
    uint32_t refcount() const noexcept
    {
      if (!ctrl_)
      {
        return 0;
      }
      return ctrl_->refcount.load(std::memory_order_relaxed);
    }

    /**
     * @brief Manually release shared ownership
     *
     * Immediately stops protecting the resource and releases this guard's
     * reference. If this was the last guard (refcount 1→0), the deactivator
     * is invoked. After reset(), this guard is in a released state and
     * subsequent destruction is a no-op.
     *
     * Use case: Explicitly releasing a resource before the guard goes out
     * of scope (e.g., to power down early when done with a peripheral).
     */
    void reset() noexcept
    {
      release();
      ctrl_ = nullptr;
    }

  private:
    control_block *ctrl_ = nullptr;

    /**
     * @brief Internal helper to release the reference
     *
     * Decrements refcount and invokes deactivator if this was the last guard.
     * Thread-safe via critical sections.
     */
    void release() noexcept
    {
      if (!ctrl_)
      {
        return;
      }

      taskENTER_CRITICAL(&ctrl_->spinlock);
      uint32_t old_refcount = ctrl_->refcount.fetch_sub(1, std::memory_order_relaxed);
      taskEXIT_CRITICAL(&ctrl_->spinlock);
      ESP_LOGD(TG, "Release shared guard %p = %lu",&ctrl_,refcount());
      // If this was the last reference, invoke the deactivator
      if (old_refcount == 1)
      {
        ESP_LOGI(TG, "calling deactivator");
        ctrl_->deactivator();
      }
    }
  };

} // namespace cjf

#endif /* C67B5364_3482_4E5B_8C0E_93FB5D9857D1 */
