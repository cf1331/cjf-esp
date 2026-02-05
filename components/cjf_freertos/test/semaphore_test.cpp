#include <cjf/freertos/semaphore.h>
#include <esp_log.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace cjf::freertos;

static const char* TAG = "test:semaphore";

TEST_CASE("Binary semaphore constructs successfully", "[freertos][semaphore]")
{
    auto sem = binary_semaphore::create(false);
    TEST_ASSERT_TRUE(sem.has_value());
    TEST_ASSERT_NOT_NULL(sem->handle());
}

TEST_CASE("Binary semaphore initial state", "[freertos][semaphore]")
{
    // Create with initial value false (not available)
    auto sem = binary_semaphore::create(false);
    TEST_ASSERT_TRUE(sem.has_value());

    // Should not be able to take immediately
    esp_err_t err = sem->take(0);
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, err);

    // Give the semaphore
    err = sem->give();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    // Now should be able to take
    err = sem->take(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

TEST_CASE("Binary semaphore given initially", "[freertos][semaphore]")
{
    auto sem = binary_semaphore::create(true);
    TEST_ASSERT_TRUE(sem.has_value());

    // Should be able to take immediately
    esp_err_t err = sem->take(0);
    TEST_ASSERT_EQUAL(ESP_OK, err);

    // Should not be able to take again
    err = sem->take(0);
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, err);
}

TEST_CASE("Binary semaphore signaling between tasks", "[freertos][semaphore]")
{
    auto sem = binary_semaphore::create(false);
    TEST_ASSERT_TRUE(sem.has_value());

    volatile bool task_executed = false;

    task<2048> worker_task(
        [&]() noexcept {
            ESP_LOGI(TAG, "Worker waiting for signal");
            esp_err_t err = sem->take(pdMS_TO_TICKS(1000));
            if (err == ESP_OK) {
                task_executed = true;
                ESP_LOGI(TAG, "Worker signaled");
            }
        },
        "worker",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(worker_task));

    // Give worker time to start waiting
    vTaskDelay(pdMS_TO_TICKS(100));

    // Signal the worker
    esp_err_t err = sem->give();
    TEST_ASSERT_EQUAL(ESP_OK, err);

    // Wait for worker to execute
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(task_executed);
}

TEST_CASE("Binary semaphore chrono timeout", "[freertos][semaphore]")
{
    using namespace std::chrono_literals;

    auto sem = binary_semaphore::create(false);
    TEST_ASSERT_TRUE(sem.has_value());

    // Try to take with chrono duration (should timeout)
    esp_err_t err = sem->take(50ms);
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, err);

    // Give and take with chrono
    sem->give();
    err = sem->take(100ms);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}
