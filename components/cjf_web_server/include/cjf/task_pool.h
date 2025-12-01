#ifndef E7A8B9C4_5F6D_4E2A_9B3C_1D8E7F4A2B5C
#define E7A8B9C4_5F6D_4E2A_9B3C_1D8E7F4A2B5C

#include <cjf/error_handling.h>
#include <cjf/freertos_raii.h>
#include <cjf/timeout.h>
#include <esp_err.h>
#include <esp_log.h>
#include <expected>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace cjf
{

  /**
   * @brief A pool of worker tasks for processing work items asynchronously
   *
   * @tparam WorkItem The type of work items to be processed. Must be copyable.
   */
  template <typename WorkItem>
  class task_pool
  {
  public:
    constexpr static const char *TAG = "cjf:task_pool";

    /// Function signature for processing work items
    using work_handler_func = std::function<void(WorkItem &item)>;

    /**
     * @brief Configuration for the task pool
     */
    struct config
    {
      size_t worker_count = 3;          ///< Number of worker tasks
      size_t queue_depth = 5;           ///< Depth of the work queue
      size_t stack_size = 4096;         ///< Stack size for worker tasks
      UBaseType_t priority = 5;         ///< Priority of worker tasks
      const char *task_name = "worker"; ///< Base name for worker tasks
      work_handler_func handler;        ///< Function to process work items
    };

    /**
     * @brief Create a new task pool with workers started
     *
     * @param cfg Configuration for the pool
     * @return Expected containing the task pool on success, or error code on failure
     */
    static std::expected<task_pool, esp_err_t> create(const config &cfg);

    /**
     * @brief Destroy the task pool, stopping all workers
     */
    ~task_pool();

    // Disable copy
    task_pool(const task_pool &) = delete;
    task_pool &operator=(const task_pool &) = delete;

    // Enable move
    task_pool(task_pool &&other) noexcept;
    task_pool &operator=(task_pool &&other) noexcept;

    /**
     * @brief Stop all worker tasks
     *
     * @param ticks_to_wait Maximum time to wait for workers to exit gracefully
     */
    void stop(TickType_t ticks_to_wait = pdMS_TO_TICKS(1000));

    /**
     * @brief Submit a work item to the pool
     *
     * @param item The work item to process
     * @param ticks_to_wait Maximum time to wait if queue is full
     * @return true if item was queued successfully
     */
    bool submit(const WorkItem &item, TickType_t ticks_to_wait = portMAX_DELAY);

  private:
    struct task_pool_context
    {
      bool running;
      queue_ptr work_queue;
      semaphore_ptr workers_done_sem;
      work_handler_func handler;
    };

    struct stop_signal
    {
    };

    using queue_item = std::variant<WorkItem, stop_signal>;

    /**
     * @brief Private constructor - use create() factory method instead
     *
     * @param cfg Configuration for the pool
     */
    explicit task_pool(std::vector<task_ptr> worker_tasks, std::unique_ptr<task_pool_context> ctx);

    std::unique_ptr<task_pool_context> ctx_;
    std::vector<task_ptr> worker_tasks_;

    static void worker_task(void *arg);
  };

  // Template implementation
  template <typename WorkItem>
  std::expected<task_pool<WorkItem>, esp_err_t> task_pool<WorkItem>::create(const config &cfg)
  {
    RETURN_UNEXPECTED_ON_FALSE(cfg.handler, ESP_ERR_INVALID_ARG, TAG);

    auto ctx = std::make_unique<task_pool_context>(
        true,
        make_queue(cfg.queue_depth, sizeof(queue_item)),
        create_semaphore(cfg.worker_count, 0),
        cfg.handler);
    RETURN_UNEXPECTED_ON_FALSE(ctx->work_queue, ESP_ERR_NO_MEM, TAG);
    RETURN_UNEXPECTED_ON_FALSE(ctx->workers_done_sem, ESP_ERR_NO_MEM, TAG);

    std::vector<task_ptr> worker_tasks;
    worker_tasks.reserve(cfg.worker_count);
    for (size_t i = 0; i < cfg.worker_count; i++)
    {
      auto task = create_task(
          worker_task,
          cfg.task_name,
          cfg.stack_size,
          ctx.get(),
          cfg.priority);
      RETURN_UNEXPECTED_ON_FALSE(task, ESP_ERR_NO_MEM, TAG);
      worker_tasks.push_back(std::move(task));
    }
    return task_pool(std::move(worker_tasks), std::move(ctx));
  }

  template <typename WorkItem>
  task_pool<WorkItem>::task_pool(std::vector<task_ptr> worker_tasks, std::unique_ptr<task_pool_context> ctx)
      : ctx_(std::move(ctx)), worker_tasks_(std::move(worker_tasks)) {}

  template <typename WorkItem>
  task_pool<WorkItem>::~task_pool()
  {
    if (worker_tasks_.size() > 0)
    {
      stop();
    }
  }

  template <typename WorkItem>
  task_pool<WorkItem>::task_pool(task_pool &&other) noexcept
      : ctx_(std::move(other.ctx_)),
        worker_tasks_(std::move(other.worker_tasks_)) {}

  template <typename WorkItem>
  task_pool<WorkItem> &task_pool<WorkItem>::operator=(task_pool &&other) noexcept
  {
    if (this != &other)
    {
      // Stop our current resources
      stop();

      // Take ownership from other
      ctx_ = std::move(other.ctx_);
      worker_tasks_ = std::move(other.worker_tasks_);
    }
    return *this;
  }

  template <typename WorkItem>
  void task_pool<WorkItem>::stop(TickType_t ticks_to_wait)
  {
    if (!ctx_ || !ctx_->running)
    {
      return;
    }
    ctx_->running = false;

    cjf::timeout timeout(ticks_to_wait);

    // Send stop signal to wake up all blocked workers
    auto work_queue = ctx_->work_queue.get();
    if (work_queue)
    {
      for (size_t i = 0; i < worker_tasks_.size(); i++)
      {
        std::variant<WorkItem, stop_signal> sentinel = stop_signal{};
        xQueueSend(work_queue, &sentinel, timeout.remaining());
      }
    }

    // Wait for workers to exit
    auto workers_done_sem = ctx_->workers_done_sem.get();
    for (size_t i = 0; i < worker_tasks_.size(); i++)
    {
      if (xSemaphoreTake(workers_done_sem, timeout.remaining()) != pdTRUE)
      {
        break; // Timeout - no more workers signaled completion
      }
    }

    // Forcefully delete any workers that haven't exited
    worker_tasks_.clear();

    // Empty the work queue
    if (work_queue)
    {
      xQueueReset(work_queue);
    }
  }

  template <typename WorkItem>
  bool task_pool<WorkItem>::submit(const WorkItem &item, TickType_t ticks_to_wait)
  {
    if (!ctx_ || !ctx_->running || !ctx_->work_queue)
    {
      return false;
    }
    // Copy the item into a queue item variant
    // This is necessary because the queue holds variants
    queue_item qi = item;
    return xQueueSend(ctx_->work_queue.get(), &qi, ticks_to_wait) == pdTRUE;
  }

  template <typename WorkItem>
  void task_pool<WorkItem>::worker_task(void *arg)
  {
    auto ctx = reinterpret_cast<task_pool<WorkItem>::task_pool_context *>(arg);
    while (ctx->running)
    {
      queue_item item;
      if (xQueueReceive(ctx->work_queue.get(), &item, portMAX_DELAY) == pdTRUE)
      {
        if (std::holds_alternative<stop_signal>(item))
        {
          break;
        }
        WorkItem work_item = std::get<WorkItem>(item);
        ctx->handler(work_item);
      }
    }
    xSemaphoreGive(ctx->workers_done_sem.get());
    vTaskDelete(nullptr);
  }
} // namespace cjf

#endif /* E7A8B9C4_5F6D_4E2A_9B3C_1D8E7F4A2B5C */
