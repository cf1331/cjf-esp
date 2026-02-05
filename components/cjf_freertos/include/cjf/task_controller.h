#ifndef B8CD9BF7_C305_487C_B5BF_C44F1A14EC7F
#define B8CD9BF7_C305_487C_B5BF_C44F1A14EC7F

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>

namespace cjf
{
  // Reusable class for controlling task execution via direct-to-task notifications
  class task_controller
  {
  public:
    task_controller();
    task_controller(TaskHandle_t handle);

    // Start the task's work loop
    void start();

    // Stop the task's work loop
    void stop();

    // Check if a start notification was received (blocks until notification)
    bool wait_for_start();

    // Check if a stop notification was received (non-blocking)
    bool should_run();

    static task_controller create_task(
        TaskFunction_t task_func,
        const char *name,
        uint16_t stack_depth,
        void *params,
        UBaseType_t priority);

    static task_controller create_task(
        const std::function<void(void)> &task_func,
        const char *name,
        uint16_t stack_depth,
        UBaseType_t priority);

  private:
    TaskHandle_t task_handle;
  };

  template <typename Optional>
  void delay_until_or_stop(TickType_t *previous_wake_time, Optional time_increment, task_controller &controller)
  {
    if (time_increment)
    {
      xTaskDelayUntil(previous_wake_time, *time_increment);
    }
    else
    {
      controller.stop();
    }
  }

} // namespace cjf

#endif /* B8CD9BF7_C305_487C_B5BF_C44F1A14EC7F */
