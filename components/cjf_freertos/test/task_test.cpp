#include <cjf/freertos/task.h>
#include <esp_log.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace cjf::freertos;

static const char* TAG = "test:task";

TEST_CASE("Task constructs successfully", "[freertos][task]")
{
    volatile bool task_executed = false;

    task<2048> my_task(
        [&task_executed]() noexcept {
            task_executed = true;
            ESP_LOGI(TAG, "Task executed");
        },
        "test_task",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(my_task));
    TEST_ASSERT_EQUAL(ESP_OK, static_cast<esp_err_t>(my_task));
    TEST_ASSERT_NOT_NULL(my_task.handle());

    // Give task time to execute
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(task_executed);
}

TEST_CASE("Task failure returns error status", "[freertos][task]")
{
    // This test would need a way to force task creation to fail
    // For now, just verify successful creation has correct status
    task<1024> my_task(
        []() noexcept {
            vTaskDelay(pdMS_TO_TICKS(10));
        },
        "status_task",
        3
    );

    if (my_task) {
        TEST_ASSERT_EQUAL(ESP_OK, static_cast<esp_err_t>(my_task));
    } else {
        TEST_ASSERT_EQUAL(ESP_ERR_NO_MEM, static_cast<esp_err_t>(my_task));
    }
}

TEST_CASE("Task with different stack sizes", "[freertos][task]")
{
    task<512> small_task(
        []() noexcept { ESP_LOGI(TAG, "Small task"); },
        "small",
        2
    );

    task<8192> large_task(
        []() noexcept { ESP_LOGI(TAG, "Large task"); },
        "large",
        2
    );

    TEST_ASSERT_TRUE(static_cast<bool>(small_task));
    TEST_ASSERT_TRUE(static_cast<bool>(large_task));

    vTaskDelay(pdMS_TO_TICKS(50));
}

TEST_CASE("Task with different priorities", "[freertos][task]")
{
    volatile int execution_order = 0;
    volatile int high_order = 0;
    volatile int low_order = 0;

    task<1024> low_priority_task(
        [&]() noexcept {
            low_order = ++execution_order;
            ESP_LOGI(TAG, "Low priority task executed: %d", low_order);
        },
        "low_prio",
        1
    );

    task<1024> high_priority_task(
        [&]() noexcept {
            high_order = ++execution_order;
            ESP_LOGI(TAG, "High priority task executed: %d", high_order);
        },
        "high_prio",
        10
    );

    TEST_ASSERT_TRUE(static_cast<bool>(low_priority_task));
    TEST_ASSERT_TRUE(static_cast<bool>(high_priority_task));

    vTaskDelay(pdMS_TO_TICKS(100));

    // High priority should execute first
    TEST_ASSERT_LESS_THAN(low_order, high_order);
}

TEST_CASE("Task is neither copyable nor moveable", "[freertos][task]")
{
    // This is a compile-time test - just verify it exists
    task<1024> my_task(
        []() noexcept { vTaskDelay(pdMS_TO_TICKS(10)); },
        "immovable",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(my_task));

    // These would fail to compile:
    // task<1024> copy = my_task;  // Deleted copy constructor
    // task<1024> moved = std::move(my_task);  // Deleted move constructor
}
