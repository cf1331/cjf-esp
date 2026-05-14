#ifndef A7254FF3_8676_4993_9573_0146D0C0430B
#define A7254FF3_8676_4993_9573_0146D0C0430B

/**
 * @file watchable.h
 * @brief Thread-safe observer pattern implementation
 */

#include <cjf/freertos/semaphore.h>
#include <list>

namespace cjf
{
  /**
   * @brief Thread-safe observer pattern for value change notifications
   *
   * Provides a reusable observer/listener pattern for safe multi-task access.
   *
   * @tparam T The observable item type passed to callbacks
   * @tparam TCtx User context type (default: `void*`)
   *
   * @note Thread-safe: Safe to use across multiple tasks.
   *
   * @code{.cpp}
   * struct sensor_data { float temp; };
   * watchable<sensor_data> sensor;
   *
   * // Register observer
   * sensor.watch([](sensor_data& data, void* ctx) {
   *     ESP_LOGI("SENSOR", "Temp: %.2f", data.temp);
   * }, nullptr);
   *
   * // Notify all observers
   * sensor_data reading{25.3f};
   * sensor.notify(reading);
   * @endcode
   */
  template <typename T, typename TCtx = void *>
  class watchable
  {
  public:
    /**
     * @brief Callback function type for notifications
     * @param item Reference to the changed item
     * @param ctx User-provided context
     */
    using value_changed_func = void (*)(T &item, TCtx ctx);

    watchable() = default;
    ~watchable() = default;

    // Non-copyable, non-movable: cjf::freertos::mutex embeds its own storage
    watchable(const watchable &) = delete;
    watchable &operator=(const watchable &) = delete;
    watchable(watchable &&) = delete;
    watchable &operator=(watchable &&) = delete;

    /**
     * @brief Check if any callbacks are registered
     * @return `true` if at least one watcher is registered
     */
    bool is_watched() const;

    /**
     * @brief Register a callback for notifications
     * @param callback Function to invoke on notify()
     * @param ctx User context passed to callback
     */
    void watch(value_changed_func callback, TCtx ctx);

    /**
     * @brief Unregister a callback
     * @param callback The callback function to remove
     */
    void unwatch(value_changed_func callback);

    /**
     * @brief Notify all registered callbacks
     * @param item Reference to the changed item
     *
     * @note Callbacks are invoked with the mutex released to prevent deadlocks.
     *       A snapshot of the watcher list is taken while holding the mutex.
     */
    void notify(T &item) const;

  private:
    struct watcher
    {
      value_changed_func callback;
      TCtx ctx;

      watcher(value_changed_func callback, TCtx ctx)
          : callback(callback), ctx(ctx)
      {
      }
    };

    mutable cjf::freertos::mutex mutex_;
    std::list<watcher> watchers_;
  };

  template <typename T, typename TCtx>
  inline bool watchable<T, TCtx>::is_watched() const
  {
    cjf::freertos::lock_guard guard{mutex_};
    return !watchers_.empty();
  }

  template <typename T, typename TCtx>
  inline void watchable<T, TCtx>::watch(value_changed_func callback, TCtx ctx)
  {
    cjf::freertos::lock_guard guard{mutex_};
    watchers_.emplace_back(callback, ctx);
  }

  template <typename T, typename TCtx>
  inline void watchable<T, TCtx>::unwatch(value_changed_func callback)
  {
    cjf::freertos::lock_guard guard{mutex_};
    watchers_.remove_if([callback](const watcher &w)
                        { return w.callback == callback; });
  }

  template <typename T, typename TCtx>
  inline void watchable<T, TCtx>::notify(T &item) const
  {
    // Snapshot the watcher list while holding the mutex, then release
    // before firing callbacks to prevent deadlocks.
    std::list<watcher> watchers_copy;
    {
      cjf::freertos::lock_guard guard{mutex_};
      watchers_copy = watchers_;
    }

    for (const auto &watcher : watchers_copy)
    {
      watcher.callback(item, watcher.ctx);
    }
  }

}

#endif /* A7254FF3_8676_4993_9573_0146D0C0430B */
