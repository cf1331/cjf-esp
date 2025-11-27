#ifndef B8F80DC8_1082_463F_A72C_DA049688EBF0
#define B8F80DC8_1082_463F_A72C_DA049688EBF0

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <memory>

namespace cjf
{

  struct queue_deleter
  {
    void operator()(QueueHandle_t queue) const;
  };

  using queue_ptr = std::unique_ptr<QueueDefinition, queue_deleter>;

  queue_ptr make_queue(size_t depth, size_t item_size);

  struct semaphore_deleter
  {
    void operator()(SemaphoreHandle_t sem) const;
  };

  using semaphore_ptr = std::unique_ptr<QueueDefinition, semaphore_deleter>;

  semaphore_ptr create_semaphore(UBaseType_t max_count, UBaseType_t initial_count);

  struct task_deleter
  {
    void operator()(TaskHandle_t task) const;
  };

  using task_ptr = std::unique_ptr<tskTaskControlBlock, task_deleter>;

  task_ptr create_task(
      TaskFunction_t task_func,
      const char *name,
      uint16_t stack_depth,
      void *params,
      UBaseType_t priority);

} // namespace cjf

#endif /* B8F80DC8_1082_463F_A72C_DA049688EBF0 */
