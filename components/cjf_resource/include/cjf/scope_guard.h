#ifndef A6950FBD_D410_4DC9_B8C8_DE108BE88AC2
#define A6950FBD_D410_4DC9_B8C8_DE108BE88AC2

/**
 * @file scope_guard.h
 * @brief RAII scope guard for cleanup actions
 *
 * This file provides a lightweight RAII wrapper for managing cleanup operations
 * that take no arguments (e.g., esp_camera_deinit(), nvs_flash_deinit()).
 * It ensures cleanup is performed on scope exit while supporting move semantics.
 */

#include <concepts>
#include <type_traits>
#include <utility>

namespace cjf
{
  /**
   * @brief RAII scope guard for automatic cleanup on scope exit
   *
   * Provides a type-safe RAII wrapper that invokes a cleanup function
   * (deleter) when the guard is destroyed. The guard supports move semantics
   * for ownership transfer and prevents copying to avoid double cleanup.
   *
   * This class is useful for wrapping C APIs that manage global state or
   * singleton resources (e.g., esp_camera, esp_driver_nvs).
   *
   * @tparam Deleter Functor type with `operator()()` taking no arguments.
   *                 Must be default constructible and stateless for optimal
   *                 memory efficiency (EBO).
   *
   * @note The deleter is invoked only once, by the owning instance's destructor.
   *
   * Example usage:
   * @code
   * class camera {
   *   struct camera_deleter {
   *     void operator()() const noexcept {
   *       esp_camera_deinit();
   *     }
   *   };
   *   cjf::scope_guard<camera_deleter> scope_guard_;
   * };
   * @endcode
   */
  template <typename Deleter>
    requires std::is_invocable_v<Deleter>
  class scope_guard
  {
  public:
    /**
     * @brief Construct an owning scope guard
     *
     * Creates a scope guard that will invoke the deleter on destruction.
     * Uses a default-constructed deleter.
     */
    scope_guard() noexcept 
      requires std::is_default_constructible_v<Deleter>
      : is_owner_(true), deleter_() {}

    /**
     * @brief Construct an owning scope guard with a deleter instance
     *
     * Creates a scope guard that will invoke the provided deleter on destruction.
     *
     * @param deleter The deleter instance to invoke on cleanup
     */
    explicit scope_guard(Deleter deleter) noexcept
      : is_owner_(true), deleter_(std::move(deleter)) {}

    // Move-only semantics: Each `scope_guard` represents exclusive ownership of a
    // cleanup action. Copying would create multiple owners that would invoke the
    // deleter multiple times, leading to double-cleanup and undefined behavior.
    scope_guard(const scope_guard &) = delete;
    scope_guard &operator=(const scope_guard &) = delete;

    /**
     * @brief Move constructor - transfers ownership
     *
     * Constructs a scope guard by transferring ownership from another guard.
     * The source guard is left in a non-owning state and will not invoke cleanup.
     *
     * @param other The source scope guard to move from
     */
    scope_guard(scope_guard &&other) noexcept
        : is_owner_(std::exchange(other.is_owner_, false)),
          deleter_(std::move(other.deleter_))
    {
    }

    /**
     * @brief Move assignment operator - transfers ownership
     *
     * Cleans up the current resource (if owned), then transfers ownership from
     * the source guard. The source guard is left in a non-owning state.
     *
     * @param other The source scope guard to move from
     * @return Reference to this scope guard
     */
    scope_guard &operator=(scope_guard &&other) noexcept
    {
      if (this != &other)
      {
        cleanup_if_owner();
        is_owner_ = std::exchange(other.is_owner_, false);
        deleter_ = std::move(other.deleter_);
      }
      return *this;
    }

    /**
     * @brief Destructor - invokes cleanup if owner
     *
     * If this guard owns the cleanup action, invokes the deleter.
     */
    ~scope_guard() noexcept
    {
      cleanup_if_owner();
    }

    /**
     * @brief Check if this guard owns the cleanup action
     *
     * @return `true` if this guard will invoke cleanup on destruction
     */
    bool is_owner() const noexcept
    {
      return is_owner_;
    }

    /**
     * @brief Manually invoke cleanup and release ownership
     *
     * Immediately invokes the deleter (if owner) and transitions to a
     * non-owning state. Subsequent destruction will not invoke cleanup.
     */
    void reset() noexcept
    {
      cleanup_if_owner();
      is_owner_ = false;
    }

    /**
     * @brief Check if this guard is active (owns cleanup)
     *
     * @return `true` if this guard will invoke cleanup on destruction
     */
    explicit operator bool() const noexcept
    {
      return is_owner();
    }

  private:
    bool is_owner_;
    Deleter deleter_;

    /**
     * @brief Internal helper to conditionally invoke cleanup
     */
    void cleanup_if_owner() noexcept
    {
      if (is_owner())
      {
        deleter_();
      }
    }
  };

} // namespace cjf

#endif // A6950FBD_D410_4DC9_B8C8_DE108BE88AC2
