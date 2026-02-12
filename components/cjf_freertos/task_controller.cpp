#include "cjf/task_controller.h"

namespace cjf
{
  task_controller::task_controller() : task_handle(nullptr) {}
  task_controller::task_controller(TaskHandle_t handle) : task_handle(handle) {}

  task_controller task_controller::create_task(
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
      return task_controller(nullptr);
    }
    return task_controller(handle);
  }

  task_controller task_controller::create_task(
      const std::function<void(void)> &task_func,
      const char *name,
      uint16_t stack_depth,
      UBaseType_t priority)
  {
    auto task_wrapper = [](void *params)
    {
      auto *task_func = reinterpret_cast<std::function<void(void)> *>(params);
      (*task_func)();
      vTaskDelete(nullptr);
    };
    return create_task(
        task_wrapper,
        name,
        stack_depth,
        static_cast<void *>(const_cast<std::function<void(void)> *>(&task_func)),
        priority);
  }

  // Start the task's work loop
  void task_controller::start()
  {
    if (!task_handle) return;
    xTaskNotify(task_handle, 1, eSetValueWithOverwrite);
  }

  // Stop the task's work loop
  void task_controller::stop()
  {
    if (!task_handle) return;
    xTaskNotify(task_handle, 0, eSetValueWithOverwrite);
  }

  // Check if a start notification was received (blocks until notification)
  bool task_controller::wait_for_start()
  {
    uint32_t notification_value = 0;
    xTaskNotifyWait(0, 0, &notification_value, portMAX_DELAY);
    return notification_value == 1;
  }

  // Check if a stop notification was received (non-blocking)
  bool task_controller::should_run()
  {
    uint32_t notification_value = 0;
    if (xTaskNotifyWait(0, 0, &notification_value, 0) == pdTRUE && notification_value == 0)
    {
      return false;
    }
    return true;
  }

  bool task_controller::start_from_isr()
  {
    if (!task_handle) return false;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(task_handle, 1, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    return xHigherPriorityTaskWoken == pdTRUE;
  }

} // namespace cjf
