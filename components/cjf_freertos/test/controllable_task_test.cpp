#include <cjf/freertos/controllable_task.h>
#include <esp_log.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace cjf::freertos;

static const char* TAG = "test:controllable_task";

TEST_CASE("Controllable task constructs successfully", "[freertos][controllable_task]")
{
    controllable_task<2048> task_ctrl(
        [&task_ctrl]() noexcept {
            ESP_LOGI(TAG, "Controllable task running");
        },
        "ctrl_task",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(task_ctrl));
    TEST_ASSERT_EQUAL(ESP_OK, static_cast<esp_err_t>(task_ctrl));
    TEST_ASSERT_NOT_NULL(task_ctrl.task.handle());

    vTaskDelay(pdMS_TO_TICKS(100));
}

TEST_CASE("Controllable task start and stop", "[freertos][controllable_task]")
{
    volatile bool task_started = false;
    volatile bool task_stopped = false;

    controllable_task<2048> task_ctrl(
        [&]() noexcept {
            task_ctrl.wait_for_start();
            task_started = true;
            ESP_LOGI(TAG, "Task started");

            while (task_ctrl.should_run()) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }

            task_stopped = true;
            ESP_LOGI(TAG, "Task stopped");
        },
        "lifecycle_task",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(task_ctrl));

    // Task should be waiting for start
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_FALSE(task_started);

    // Start the task
    task_ctrl.start();
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(task_started);
    TEST_ASSERT_FALSE(task_stopped);

    // Stop the task
    task_ctrl.stop();
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(task_stopped);
}

TEST_CASE("Controllable task work loop", "[freertos][controllable_task]")
{
    volatile int work_count = 0;

    controllable_task<2048> task_ctrl(
        [&]() noexcept {
            task_ctrl.wait_for_start();

            while (task_ctrl.should_run()) {
                work_count++;
                vTaskDelay(pdMS_TO_TICKS(10));
            }

            ESP_LOGI(TAG, "Work completed: %d iterations", work_count);
        },
        "worker_task",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(task_ctrl));

    // Start task and let it work
    task_ctrl.start();
    vTaskDelay(pdMS_TO_TICKS(100));

    // Stop task
    task_ctrl.stop();
    vTaskDelay(pdMS_TO_TICKS(100));

    // Task should have done some work
    TEST_ASSERT_GREATER_THAN(0, work_count);
    ESP_LOGI(TAG, "Task performed %d iterations", work_count);
}

TEST_CASE("Controllable task controller access", "[freertos][controllable_task]")
{
    volatile bool task_running = false;

    controllable_task<2048> task_ctrl(
        [&]() noexcept {
            task_ctrl.wait_for_start();
            task_running = true;

            while (task_ctrl.should_run()) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        },
        "access_task",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(task_ctrl));

    // Access controller directly
    task_ctrl.controller.start();
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(task_running);

    task_ctrl.controller.stop();
    vTaskDelay(pdMS_TO_TICKS(100));
}

TEST_CASE("Controllable task forwarding methods", "[freertos][controllable_task]")
{
    volatile int state = 0;  // 0=init, 1=started, 2=stopped

    controllable_task<2048> task_ctrl(
        [&]() noexcept {
            bool started = task_ctrl.wait_for_start();
            if (started) {
                state = 1;
            }

            while (task_ctrl.should_run()) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }

            state = 2;
        },
        "forward_task",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(task_ctrl));
    TEST_ASSERT_EQUAL(0, state);

    // Use forwarding methods instead of accessing controller directly
    task_ctrl.start();
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(1, state);

    task_ctrl.stop();
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(2, state);
}

TEST_CASE("Controllable task multiple start/stop cycles", "[freertos][controllable_task]")
{
    volatile int cycle_count = 0;

    controllable_task<2048> task_ctrl(
        [&]() noexcept {
            for (int i = 0; i < 3; i++) {
                task_ctrl.wait_for_start();
                cycle_count++;

                while (task_ctrl.should_run()) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        },
        "cycle_task",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(task_ctrl));

    // Run 3 cycles
    for (int i = 0; i < 3; i++) {
        task_ctrl.start();
        vTaskDelay(pdMS_TO_TICKS(50));
        TEST_ASSERT_EQUAL(i + 1, cycle_count);

        task_ctrl.stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    TEST_ASSERT_EQUAL(3, cycle_count);
}

TEST_CASE("Controllable task is neither copyable nor moveable", "[freertos][controllable_task]")
{
    // This is a compile-time test - just verify it exists
    controllable_task<1024> task_ctrl(
        [&task_ctrl]() noexcept {
            task_ctrl.wait_for_start();
            vTaskDelay(pdMS_TO_TICKS(10));
        },
        "immovable",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(task_ctrl));

    // These would fail to compile:
    // controllable_task<1024> copy = task_ctrl;  // Deleted
    // controllable_task<1024> moved = std::move(task_ctrl);  // Deleted
}
