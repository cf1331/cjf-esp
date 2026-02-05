/**
 * @file test_freertos_wrappers.cpp
 * @brief Test/example usage of FreeRTOS RAII wrappers
 */

#include <cjf/freertos.h>
#include <esp_log.h>

static const char* TAG = "freertos_test";

namespace {

// Example: Simple task without controller
void test_basic_task() {
    auto task_result = cjf::freertos::task<2048>::create(
        []() noexcept {
            ESP_LOGI(TAG, "Task running");
            vTaskDelay(pdMS_TO_TICKS(1000));
        },
        "basic_task",
        5
    );

    if (task_result) {
        ESP_LOGI(TAG, "Created basic task successfully");
    } else {
        ESP_LOGE(TAG, "Failed to create basic task: %d", task_result.error());
    }
}

// Example: Task with controller for lifecycle management
void test_task_with_controller() {
    // Note: The controller is accessible via task_result->controller after creation
    // The lambda captures it by reference in actual usage

    auto task_result = cjf::freertos::task_with_controller<4096>::create(
        []() noexcept {
            ESP_LOGI(TAG, "Controlled task created");
            // In real usage, task function would access controller through closure
            vTaskDelay(pdMS_TO_TICKS(100));
        },
        "controlled_task",
        5
    );

    if (task_result) {
        ESP_LOGI(TAG, "Created controlled task successfully");

        // Start the task
        task_result->controller.start();
        vTaskDelay(pdMS_TO_TICKS(500));

        // Stop the task
        task_result->controller.stop();
    } else {
        ESP_LOGE(TAG, "Failed to create controlled task: %d", task_result.error());
    }
}

// Example: Queue for passing data between tasks
struct sensor_data {
    uint32_t timestamp;
    float value;
};

void test_queue() {
    auto queue_result = cjf::freertos::queue<sensor_data, 10>::create();

    if (queue_result) {
        ESP_LOGI(TAG, "Created queue successfully");

        // Send data
        sensor_data data{.timestamp = 12345, .value = 23.5f};
        esp_err_t err = queue_result->send(data, pdMS_TO_TICKS(100));

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Sent data to queue");

            // Receive data
            sensor_data received;
            err = queue_result->receive(received, pdMS_TO_TICKS(100));

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Received data: timestamp=%lu, value=%.2f",
                        received.timestamp, received.value);
            }
        }
    } else {
        ESP_LOGE(TAG, "Failed to create queue: %d", queue_result.error());
    }
}

// Example: Binary semaphore for signaling
void test_semaphore() {
    auto sem_result = cjf::freertos::binary_semaphore::create(false);

    if (sem_result) {
        ESP_LOGI(TAG, "Created binary semaphore successfully");

        // Try to take (should timeout immediately)
        esp_err_t err = sem_result->take(0);
        ESP_LOGI(TAG, "Take (should fail): %s", err == ESP_OK ? "OK" : "TIMEOUT");

        // Give semaphore
        err = sem_result->give();
        ESP_LOGI(TAG, "Give: %s", err == ESP_OK ? "OK" : "FAILED");

        // Take again (should succeed)
        err = sem_result->take(0);
        ESP_LOGI(TAG, "Take (should succeed): %s", err == ESP_OK ? "OK" : "TIMEOUT");
    } else {
        ESP_LOGE(TAG, "Failed to create semaphore: %d", sem_result.error());
    }
}

// Example: Mutex for resource protection
void test_mutex() {
    auto mutex_result = cjf::freertos::mutex::create();

    if (mutex_result) {
        ESP_LOGI(TAG, "Created mutex successfully");

        // Lock with RAII guard
        {
            cjf::freertos::lock_guard lock(*mutex_result, pdMS_TO_TICKS(100));
            if (lock.locked()) {
                ESP_LOGI(TAG, "Mutex locked");
                // Critical section
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            // Mutex automatically unlocked when lock_guard destroyed
        }
        ESP_LOGI(TAG, "Mutex unlocked");
    } else {
        ESP_LOGE(TAG, "Failed to create mutex: %d", mutex_result.error());
    }
}

// Example: Event group for multi-bit synchronization
void test_event_group() {
    auto eg_result = cjf::freertos::event_group::create();

    if (eg_result) {
        ESP_LOGI(TAG, "Created event group successfully");

        // Set some bits
        constexpr EventBits_t BIT_0 = (1 << 0);
        constexpr EventBits_t BIT_1 = (1 << 1);
        constexpr EventBits_t BIT_2 = (1 << 2);

        eg_result->set(BIT_0 | BIT_1);
        ESP_LOGI(TAG, "Set bits 0 and 1");

        // Wait for any bit
        auto wait_result = eg_result->wait(BIT_0 | BIT_1 | BIT_2, false, false, 0);
        if (wait_result) {
            ESP_LOGI(TAG, "Wait succeeded, bits: 0x%lx", *wait_result);
        }

        // Clear bits
        eg_result->clear(BIT_0 | BIT_1);
        ESP_LOGI(TAG, "Cleared bits 0 and 1");
    } else {
        ESP_LOGE(TAG, "Failed to create event group: %d", eg_result.error());
    }
}

} // anonymous namespace

extern "C" void test_freertos_wrappers() {
    ESP_LOGI(TAG, "=== Testing FreeRTOS RAII Wrappers ===");

    test_basic_task();
    test_task_with_controller();
    test_queue();
    test_semaphore();
    test_mutex();
    test_event_group();

    ESP_LOGI(TAG, "=== Tests Complete ===");
}
