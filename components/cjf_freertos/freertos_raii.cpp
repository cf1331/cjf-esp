#include "cjf/freertos_raii.h"

namespace cjf
{

  void queue_deleter::operator()(QueueHandle_t queue) const
  {
    if (queue)
    {
      vQueueDelete(queue);
    }
  }

  queue_ptr make_queue(size_t depth, size_t item_size)
  {
    return queue_ptr(xQueueCreate(depth, item_size));
  }

  void semaphore_deleter::operator()(SemaphoreHandle_t sem) const
  {
    if (sem)
    {
      vSemaphoreDelete(sem);
    }
  }

  semaphore_ptr create_semaphore(UBaseType_t max_count, UBaseType_t initial_count)
  {
    return semaphore_ptr(xSemaphoreCreateCounting(max_count, initial_count));
  }

  void task_deleter::operator()(TaskHandle_t task) const
  {
    if (task)
    {
      vTaskDelete(task);
    }
  }

  task_ptr create_task(
      TaskFunction_t task_func,
      const char *name,
      uint16_t stack_depth,
      void *params,
      UBaseType_t priority)
  {
    TaskHandle_t handle = nullptr;
    BaseType_t result = xTaskCreate(
        task_func,
        name,
        stack_depth,
        params,
        priority,
        &handle);
    if (result != pdPASS || !handle)
    {
      return nullptr;
    }
    return task_ptr(handle);
  }

} // namespace cjf
