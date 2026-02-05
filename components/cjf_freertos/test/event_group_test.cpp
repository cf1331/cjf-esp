#include <cjf/freertos/event_group.h>
#include <esp_log.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace cjf::freertos;

static const char* TAG = "test:event_group";

constexpr EventBits_t BIT_0 = (1 << 0);
constexpr EventBits_t BIT_1 = (1 << 1);
constexpr EventBits_t BIT_2 = (1 << 2);
constexpr EventBits_t BIT_3 = (1 << 3);

TEST_CASE("Event group constructs successfully", "[freertos][event_group]")
{
    auto eg = event_group::create();
    TEST_ASSERT_TRUE(eg.has_value());
    TEST_ASSERT_NOT_NULL(eg->handle());
}

TEST_CASE("Event group set and wait for bits", "[freertos][event_group]")
{
    auto eg = event_group::create();
    TEST_ASSERT_TRUE(eg.has_value());

    // Initially no bits set
    EventBits_t bits = eg->get();
    TEST_ASSERT_EQUAL(0, bits);

    // Set bit 0
    eg->set(BIT_0);

    // Wait for bit 0 (should return immediately)
    auto result = eg->wait_any(BIT_0, true, pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(result.has_value());
    TEST_ASSERT_EQUAL(BIT_0, result.value());

    // Bit should be cleared (auto-clear was true)
    bits = eg->get();
    TEST_ASSERT_EQUAL(0, bits);
}

TEST_CASE("Event group wait for all bits", "[freertos][event_group]")
{
    auto eg = event_group::create();
    TEST_ASSERT_TRUE(eg.has_value());

    // Set only bit 0
    eg->set(BIT_0);

    // Wait for bits 0 and 1 (should timeout - only 0 is set)
    auto result = eg->wait_all(BIT_0 | BIT_1, false, 0);
    TEST_ASSERT_FALSE(result.has_value());
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, result.error());

    // Now set bit 1
    eg->set(BIT_1);

    // Wait for both bits (should succeed)
    result = eg->wait_all(BIT_0 | BIT_1, true, pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(result.has_value());
    TEST_ASSERT_EQUAL(BIT_0 | BIT_1, result.value() & (BIT_0 | BIT_1));

    // Both bits should be cleared
    EventBits_t bits = eg->get();
    TEST_ASSERT_EQUAL(0, bits & (BIT_0 | BIT_1));
}

TEST_CASE("Event group clear bits", "[freertos][event_group]")
{
    auto eg = event_group::create();
    TEST_ASSERT_TRUE(eg.has_value());

    // Set multiple bits
    eg->set(BIT_0 | BIT_1 | BIT_2);

    EventBits_t bits = eg->get();
    TEST_ASSERT_EQUAL(BIT_0 | BIT_1 | BIT_2, bits);

    // Clear bit 1
    eg->clear(BIT_1);

    bits = eg->get();
    TEST_ASSERT_EQUAL(BIT_0 | BIT_2, bits);
    TEST_ASSERT_EQUAL(0, bits & BIT_1);
}

TEST_CASE("Event group sync", "[freertos][event_group]")
{
    auto eg = event_group::create();
    TEST_ASSERT_TRUE(eg.has_value());

    volatile int task1_synced = 0;
    volatile int task2_synced = 0;

    // Task 1 sets BIT_0 and waits for BIT_1
    task<2048> task1(
        [&]() noexcept {
            ESP_LOGI(TAG, "Task 1 syncing...");
            auto result = eg->sync(BIT_0, BIT_0 | BIT_1, portMAX_DELAY);
            if (result.has_value()) {
                task1_synced = 1;
                ESP_LOGI(TAG, "Task 1 synced");
            }
        },
        "sync_task1",
        5
    );

    // Task 2 sets BIT_1 and waits for BIT_0
    task<2048> task2(
        [&]() noexcept {
            vTaskDelay(pdMS_TO_TICKS(100));  // Delay to ensure task 1 is waiting
            ESP_LOGI(TAG, "Task 2 syncing...");
            auto result = eg->sync(BIT_1, BIT_0 | BIT_1, portMAX_DELAY);
            if (result.has_value()) {
                task2_synced = 1;
                ESP_LOGI(TAG, "Task 2 synced");
            }
        },
        "sync_task2",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(task1));
    TEST_ASSERT_TRUE(static_cast<bool>(task2));

    // Wait for sync to complete
    vTaskDelay(pdMS_TO_TICKS(500));

    TEST_ASSERT_EQUAL(1, task1_synced);
    TEST_ASSERT_EQUAL(1, task2_synced);
}

TEST_CASE("Event group multiple bits wait any", "[freertos][event_group]")
{
    auto eg = event_group::create();
    TEST_ASSERT_TRUE(eg.has_value());

    // Set bit 2
    eg->set(BIT_2);

    // Wait for any of bits 0, 1, or 2
    auto result = eg->wait_any(BIT_0 | BIT_1 | BIT_2, false, pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(result.has_value());

    // Should have bit 2 set (might have others too)
    TEST_ASSERT_NOT_EQUAL(0, result.value() & BIT_2);
}

TEST_CASE("Event group chrono timeout", "[freertos][event_group]")
{
    using namespace std::chrono_literals;

    auto eg = event_group::create();
    TEST_ASSERT_TRUE(eg.has_value());

    // Wait for bit that's not set (should timeout)
    auto result = eg->wait_any(BIT_0, false, 50ms);
    TEST_ASSERT_FALSE(result.has_value());
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, result.error());

    // Set bit and wait with chrono
    eg->set(BIT_0);
    result = eg->wait_any(BIT_0, true, 100ms);
    TEST_ASSERT_TRUE(result.has_value());
}

TEST_CASE("Event group signaling between tasks", "[freertos][event_group]")
{
    auto eg = event_group::create();
    TEST_ASSERT_TRUE(eg.has_value());

    volatile bool worker_executed = false;

    task<2048> worker_task(
        [&]() noexcept {
            ESP_LOGI(TAG, "Worker waiting for signal");
            auto result = eg->wait_any(BIT_0, true, pdMS_TO_TICKS(1000));
            if (result.has_value()) {
                worker_executed = true;
                ESP_LOGI(TAG, "Worker received signal");
            }
        },
        "worker",
        5
    );

    TEST_ASSERT_TRUE(static_cast<bool>(worker_task));

    // Give worker time to start waiting
    vTaskDelay(pdMS_TO_TICKS(100));

    // Signal the worker
    eg->set(BIT_0);

    // Wait for worker to execute
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(worker_executed);
}
