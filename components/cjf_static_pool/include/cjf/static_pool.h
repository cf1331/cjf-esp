#ifndef B982451D_E01F_4B51_BA1D_60A754E3063C
#define B982451D_E01F_4B51_BA1D_60A754E3063C

/**
 * @file static_pool.h
 * @brief Compile-time sized object pool with zero heap allocation
 *
 * Provides a fixed-capacity object pool using static storage with stable pointer
 * guarantees and RAII-managed lifetime via unique_ptr with custom deleter.
 */

#include <array>
#include <cstddef>
#include <memory>
#include <optional>

namespace cjf
{

  /**
   * @brief Fixed-capacity object pool using compile-time allocated storage
   *
   * Provides a pool of objects with compile-time known capacity, ensuring zero heap
   * allocation and deterministic memory usage. Objects are constructed in-place using
   * std::optional storage and returned as unique_ptr with automatic cleanup.
   *
   * Features:
   * - Compile-time memory verification (size known at build time)
   * - Stable pointers (slots never relocate)
   * - RAII lifetime management via unique_ptr
   * - Works with move-only types
   * - No heap fragmentation risk
   * - Thread-safe per-instance (no shared state between pool instances)
   *
   * @tparam T Object type (must be move-constructible)
   * @tparam N Maximum capacity (allocated at compile-time)
   */
  template <typename T, size_t N>
  class static_pool
  {
  public:
    /**
     * @brief Custom deleter that returns instances to the pool
     *
     * Automatically called when unique_ptr goes out of scope, returning the
     * instance to the pool and calling its destructor.
     */
    class deleter
    {
    public:
      /**
       * @brief Construct deleter with reference to pool
       * @param pool Pointer to the pool that owns the instance
       */
      explicit deleter(static_pool *pool) noexcept : pool_(pool) {}

      /**
       * @brief Return instance to pool and destroy it
       * @param instance Pointer to instance to destroy
       */
      void operator()(T *instance) const noexcept
      {
        if (pool_ && instance)
        {
          pool_->destroy(instance);
        }
      }

    private:
      static_pool *pool_;
    };

    /**
     * @brief Type alias for unique_ptr with pool deleter
     */
    using unique_ptr = std::unique_ptr<T, deleter>;

    /**
     * @brief Create instance in first available slot
     *
     * Constructs an object in the first available slot using perfect forwarding.
     * Returns a unique_ptr that automatically returns the instance to the pool
     * when it goes out of scope.
     *
     * @tparam Args Constructor argument types
     * @param args Arguments to forward to T's constructor
     * @return unique_ptr to created instance, or nullptr if pool is full
     *
     * @note The returned unique_ptr ensures automatic cleanup - no manual destroy() needed
     * @note If pool is full, returns unique_ptr containing nullptr
     */
    template <typename... Args>
    [[nodiscard]] unique_ptr create(Args &&...args)
    {
      for (auto &slot : slots_)
      {
        if (!slot.has_value())
        {
          T *ptr = &(slot.emplace(std::forward<Args>(args)...));
          return unique_ptr(ptr, deleter(this));
        }
      }
      return unique_ptr(nullptr, deleter(this)); // No available slots
    }

    /**
     * @brief Get the maximum capacity of the pool
     * @return Maximum number of instances (N)
     */
    [[nodiscard]] constexpr size_t capacity() const noexcept { return N; }

    /**
     * @brief Check if the pool is full
     * @return true if all slots are occupied, false otherwise
     */
    [[nodiscard]] bool full() const noexcept { return size() == N; }

    /**
     * @brief Get the current number of active instances
     * @return Number of currently allocated instances
     */
    [[nodiscard]] size_t size() const noexcept
    {
      size_t count = 0;
      for (const auto &slot : slots_)
      {
        if (slot.has_value())
          ++count;
      }
      return count;
    }

  private:
    // Allow deleter to access destroy()
    friend class deleter;

    /**
     * @brief Destroy instance and free slot (private - only callable by deleter)
     * @param instance Pointer to instance to destroy
     */
    void destroy(T *instance) noexcept
    {
      for (auto &slot : slots_)
      {
        if (slot.has_value() && &(*slot) == instance)
        {
          slot.reset();
          return;
        }
      }
    }

    /// Static storage for pool slots (compile-time sized, no heap allocation)
    std::array<std::optional<T>, N> slots_;
  };

}

#endif /* B982451D_E01F_4B51_BA1D_60A754E3063C */
