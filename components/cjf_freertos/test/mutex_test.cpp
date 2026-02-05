#include <cjf/freertos/semaphore.h>
#include <esp_log.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace cjf::freertos;

static const char* TAG = "test:mutex";

TEST_CASE("Mutex constructs successfully", "[freertos][mutex]")
{
    auto mtx = mutex::create();
    TEST_ASSERT_TRUE(mtx.has_value());
    TEST_ASSERT_NOT_NULL(mtx->handle());
}

TEST_CASE("Mutex basic lock and unlock", "[freertos][mutex]")
{
    auto mtx = mutex::create();
    TEST_ASSERT_TRUE(mtx.has_value());

    // Take mutex
    esp_err_t err = mtx->lock(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);

    // Unlock mutex
    err = mtx->unlock();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

TEST_CASE("Mutex blocks on second take", "[freertos][mutex]")
{
    auto mtx = mutex::create();
    TEST_ASSERT_TRUE(mtx.has_value());

    // Take mutex
    esp_err_t err = mtx->lock(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);

    // Try to take again without unlocking (should timeout)
    err = mtx->lock(0);
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, err);

    // Unlock and try again
    mtx->unlock();
    err = mtx->lock(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);
    mtx->unlock();
}

TEST_CASE("Mutex RAII lock_guard", "[freertos][mutex]")
{
    auto mtx = mutex::create();
    TEST_ASSERT_TRUE(mtx.has_value());

    // Create scope for lock_guard
    {
        lock_guard guard(*mtx, pdMS_TO_TICKS(100));
        TEST_ASSERT_TRUE(guard.locked());

        // Mutex is locked in this scope
        // Try to lock from same task should fail
        esp_err_t err = mtx->lock(0);
        TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, err);

    }  // lock_guard destructor unlocks mutex

    // Mutex should be unlocked now
    esp_err_t err = mtx->lock(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);
    mtx->unlock();
}

TEST_CASE("Mutex protects shared resource", "[freertos][mutex]")
{
    auto mtx = mutex::create();
    TEST_ASSERT_TRUE(mtx.has_value());

    volatile int shared_counter = 0;
    volatile bool tasks_done = false;

    // Task 1 increments counter
    task<2048> task1(
        [&]() noexcept {
            for (int i = 0; i < 100; i++) {
                lock_guard guard(*mtx, portMAX_DELAY);
                shared_counter++;
            }
            ESP_LOGI(TAG, "Task 1 done");
        },
        "task1",
        5
    );

    // Task 2 increments counter
    task<2048> task2(
        [&]() noexcept {
            for (int i = 0; i < 100; i++) {
                lock_guard guard(*mtx, portMAX_DELAY);
                shared_counter++;
            }
            ESP_LOGI(TAG, "Task 2 done");
            tasks_done = true;
        },
        "task2",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(task1));
    TEST_ASSERT_TRUE(static_cast<bool>(task2));

    // Wait for tasks to complete
    while (!tasks_done) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Counter should be exactly 200 (protected by mutex)
    TEST_ASSERT_EQUAL(200, shared_counter);
}

TEST_CASE("Mutex chrono timeout", "[freertos][mutex]")
{
    using namespace std::chrono_literals;

    auto mtx = mutex::create();
    TEST_ASSERT_TRUE(mtx.has_value());

    // Lock with chrono duration
    esp_err_t err = mtx->lock(100ms);
    TEST_ASSERT_EQUAL(ESP_OK, err);

    // Try to lock again (should timeout)
    err = mtx->lock(50ms);
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, err);

    mtx->unlock();
}

TEST_CASE("Lock guard try_lock on construction", "[freertos][mutex]")
{
    auto mtx = mutex::create();
    TEST_ASSERT_TRUE(mtx.has_value());

    // First guard locks successfully
    lock_guard guard1(*mtx, 0);
    TEST_ASSERT_TRUE(guard1.locked());

    // Second guard fails to lock (mutex already held)
    lock_guard guard2(*mtx, 0);
    TEST_ASSERT_FALSE(guard2.locked());
}
