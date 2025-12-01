#ifndef A7254FF3_8676_4993_9573_0146D0C0430B
#define A7254FF3_8676_4993_9573_0146D0C0430B

/**
 * @file watchable.h
 * @brief Thread-safe observer pattern implementation for FreeRTOS
 */

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <list>
#include <memory>

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

    /**
     * @brief Construct a watchable
     */
    watchable();

    /**
     * @brief Destroy watchable
     */
    ~watchable();

    /**
     * @brief Copy constructor
     * @param other Source watchable to copy from
     */
    watchable(const watchable& other);

    /**
     * @brief Copy assignment
     * @param other Source watchable to copy from
     * @return Reference to this object
     */
    watchable& operator=(const watchable& other);

    /**
     * @brief Move constructor
     * @param other Source watchable (nullified after move)
     */
    watchable(watchable&& other) noexcept;

    /**
     * @brief Move assignment
     * @param other Source watchable (nullified after move)
     * @return Reference to this object
     */
    watchable& operator=(watchable&& other) noexcept;

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

      watcher (value_changed_func callback, TCtx ctx)
          : callback(callback), ctx(ctx)
      {
      }
    };

    mutable SemaphoreHandle_t mutex_;
    std::list<watcher> watchers_;
  };

  template <typename T, typename TCtx>
  inline watchable<T, TCtx>::watchable()
      : mutex_(xSemaphoreCreateMutex())
  {
  }

  template <typename T, typename TCtx>
  inline watchable<T, TCtx>::~watchable()
  {
    if (mutex_ != nullptr)
    {
      vSemaphoreDelete(mutex_);
    }
  }

  template <typename T, typename TCtx>
  inline watchable<T, TCtx>::watchable(const watchable& other)
      : mutex_(xSemaphoreCreateMutex())
  {
    // Copy the watcher list from other while holding its mutex
    if (other.mutex_ != nullptr && xSemaphoreTake(other.mutex_, portMAX_DELAY) == pdTRUE)
    {
      watchers_ = other.watchers_;
      xSemaphoreGive(other.mutex_);
    }
  }

  template <typename T, typename TCtx>
  inline watchable<T, TCtx>& watchable<T, TCtx>::operator=(const watchable& other)
  {
    if (this != &other)
    {
      // Lock both mutexes (in consistent order to avoid deadlock)
      if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE)
      {
        if (other.mutex_ != nullptr && xSemaphoreTake(other.mutex_, portMAX_DELAY) == pdTRUE)
        {
          watchers_ = other.watchers_;
          xSemaphoreGive(other.mutex_);
        }
        xSemaphoreGive(mutex_);
      }
    }
    return *this;
  }

  template <typename T, typename TCtx>
  inline watchable<T, TCtx>::watchable(watchable&& other) noexcept
      : mutex_(other.mutex_), watchers_(std::move(other.watchers_))
  {
    other.mutex_ = nullptr;
  }

  template <typename T, typename TCtx>
  inline watchable<T, TCtx>& watchable<T, TCtx>::operator=(watchable&& other) noexcept
  {
    if (this != &other)
    {
      // Delete our mutex
      if (mutex_ != nullptr)
      {
        vSemaphoreDelete(mutex_);
      }

      // Take ownership of other's resources
      mutex_ = other.mutex_;
      watchers_ = std::move(other.watchers_);
      other.mutex_ = nullptr;
    }
    return *this;
  }

  template <typename T, typename TCtx>
  inline bool watchable<T, TCtx>::is_watched() const
  {
    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE)
    {
      return false;
    }
    bool result = !watchers_.empty();
    xSemaphoreGive(mutex_);
    return result;
  }

  template <typename T, typename TCtx>
  inline void watchable<T, TCtx>::watch(value_changed_func callback, TCtx ctx)
  {
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE)
    {
      watchers_.emplace_back(callback, ctx);
      xSemaphoreGive(mutex_);
    }
  }

  template <typename T, typename TCtx>
  inline void watchable<T, TCtx>::unwatch(value_changed_func callback)
  {
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE)
    {
      watchers_.remove_if([callback](const watcher &w)
                          { return w.callback == callback; });
      xSemaphoreGive(mutex_);
    }
  }

  template <typename T, typename TCtx>
  inline void watchable<T, TCtx>::notify(T &item) const
  {
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE)
    {
      // Copy the watcher list while holding the mutex to avoid holding
      // the lock during callback execution (prevents deadlocks)
      std::list<watcher> watchers_copy = watchers_;
      xSemaphoreGive(mutex_);

      // Execute callbacks without holding the mutex
      for (const auto &watcher : watchers_copy)
      {
        watcher.callback(item, watcher.ctx);
      }
    }
  }

}

#endif /* A7254FF3_8676_4993_9573_0146D0C0430B */
