# FreeRTOS RAII Wrappers

This component provides statically-allocated RAII wrappers for FreeRTOS primitives, eliminating heap usage and ensuring proper resource cleanup.

## Features

- **Zero heap allocation**: All wrappers use `*CreateStatic()` APIs with embedded storage
- **Move-only semantics**: RAII ensures single ownership and automatic cleanup
- **Type safety**: Compile-time enforcement via C++23 concepts and `requires` clauses
- **`std::expected` returns**: All factory methods return `std::expected<T, esp_err_t>`
- **Convenience overloads**: `std::chrono::milliseconds` for timeout parameters

## Components

### `task<StackSize>`

Statically-allocated task wrapper with embedded stack storage.

```cpp
auto task_result = cjf::freertos::task<4096>::create(
    []() noexcept {
        ESP_LOGI("TAG", "Task running");
        vTaskDelay(pdMS_TO_TICKS(1000));
    },
    "my_task",
    5  // priority
);

if (task_result) {
    ESP_LOGI("TAG", "Task created successfully");
    // Task runs automatically, destroyed when task_result goes out of scope
}
```

**Key Features:**
- Template parameter `StackSize` is in **words** (not bytes)
- Task function must be `noexcept` (compile-time enforced via `requires` clause)
- Uses `std::function<void()>` for type erasure (fallback from `std::move_only_function`)
- Automatic task deletion on destructor

### `task_with_controller<StackSize>`

Combines `task` with `cjf::task_controller` for cooperative lifecycle management.

```cpp
auto task_result = cjf::freertos::task_with_controller<4096>::create(
    [&controller = task_result->controller]() noexcept {
        controller.wait_for_start();

        while (controller.should_run()) {
            // Task work
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    },
    "controlled_task",
    5
);

if (task_result) {
    task_result->controller.start();  // Signal task to start
    vTaskDelay(pdMS_TO_TICKS(500));
    task_result->controller.stop();   // Signal task to stop
}
```

**Use Cases:**
- Tasks that need explicit start/stop signaling
- Cooperative shutdown for graceful cleanup
- Services with controllable background work

### `queue<ItemType, QueueLength>`

Type-safe queue with embedded storage.

```cpp
struct sensor_data {
    uint32_t timestamp;
    float value;
};

auto queue_result = cjf::freertos::queue<sensor_data, 10>::create();

if (queue_result) {
    sensor_data data{.timestamp = 12345, .value = 23.5f};

    // Send (non-blocking)
    queue_result->send(data, 0);

    // Send (with timeout)
    queue_result->send(data, std::chrono::milliseconds(100));

    // Receive
    sensor_data received;
    if (queue_result->receive(received, std::chrono::milliseconds(100)) == ESP_OK) {
        // Use received data
    }

    // Query state
    size_t count = queue_result->size();
    bool is_full = queue_result->full();
}
```

**Requirements:**
- `ItemType` must be `std::is_trivially_destructible_v` (enforced via `requires` clause)
- Items are copied into/out of queue

**Methods:**
- `send(item, timeout)` - Send to back
- `send_to_front(item, timeout)` - Send to front
- `receive(item, timeout)` - Receive from front
- `peek(item, timeout)` - Peek without removing
- `size()`, `available()`, `empty()`, `full()` - Query state
- `reset()` - Clear queue

### `binary_semaphore`

Binary semaphore for task/ISR signaling.

```cpp
auto sem_result = cjf::freertos::binary_semaphore::create(false);  // starts taken

if (sem_result) {
    // Give from task
    sem_result->give();

    // Give from ISR
    bool woken = false;
    sem_result->give_from_isr(woken);
    if (woken) {
        portYIELD_FROM_ISR();
    }

    // Take (blocking)
    if (sem_result->take(portMAX_DELAY) == ESP_OK) {
        // Semaphore acquired
    }

    // Take with timeout
    if (sem_result->take(std::chrono::milliseconds(100)) == ESP_OK) {
        // Acquired within 100ms
    }
}
```

**Use Cases:**
- ISR-to-task signaling
- Simple synchronization between tasks
- Resource availability notification

### `mutex`

Mutex for protecting shared resources between tasks (not ISRs).

```cpp
auto mutex_result = cjf::freertos::mutex::create();

if (mutex_result) {
    // Manual lock/unlock
    if (mutex_result->lock(std::chrono::milliseconds(100)) == ESP_OK) {
        // Critical section
        mutex_result->unlock();
    }

    // RAII lock guard (preferred)
    {
        cjf::freertos::lock_guard lock(*mutex_result);
        if (lock.locked()) {
            // Critical section
            // Mutex automatically unlocked when lock_guard destroyed
        }
    }
}
```

**Features:**
- Priority inheritance to prevent priority inversion
- Must unlock from same task that locked
- **Not ISR-safe** (use semaphores for ISR synchronization)

### `event_group`

Multi-bit event flags for complex synchronization patterns.

```cpp
auto eg_result = cjf::freertos::event_group::create();

if (eg_result) {
    constexpr EventBits_t BIT_0 = (1 << 0);
    constexpr EventBits_t BIT_1 = (1 << 1);
    constexpr EventBits_t BIT_2 = (1 << 2);

    // Set bits
    eg_result->set(BIT_0 | BIT_1);

    // Set from ISR
    bool woken = false;
    eg_result->set_from_isr(BIT_2, woken);

    // Wait for ANY bit (non-blocking)
    auto result = eg_result->wait(BIT_0 | BIT_1 | BIT_2, false, false, 0);
    if (result) {
        ESP_LOGI("TAG", "Bits set: 0x%lx", *result);
    }

    // Wait for ALL bits (blocking, clear on exit)
    result = eg_result->wait(
        BIT_0 | BIT_1,
        true,  // clear_on_exit
        true,  // wait_for_all
        portMAX_DELAY
    );

    // Barrier synchronization
    result = eg_result->sync(
        BIT_0,           // bits this task sets
        BIT_0 | BIT_1,   // bits to wait for from all tasks
        portMAX_DELAY
    );

    // Query state
    EventBits_t current = eg_result->get();
}
```

**Use Cases:**
- Waiting for multiple conditions
- Coordinating multiple tasks
- Barrier synchronization
- Complex state machines

**Limits:**
- 24 usable bits (bits 0-23)
- Bits 24-31 reserved by FreeRTOS

## Design Patterns

### Factory Pattern

All wrappers use static factory methods returning `std::expected`:

```cpp
auto result = cjf::freertos::Type::create(/* args */);

if (result) {
    // Success - use *result
    result->method();
} else {
    // Failure - check result.error()
    ESP_LOGE("TAG", "Creation failed: %d", result.error());
}
```

### Move-Only Semantics

```cpp
// ✅ Move construction
auto queue1 = cjf::freertos::queue<int, 10>::create();
auto queue2 = std::move(*queue1);

// ✅ Move assignment
auto queue3 = cjf::freertos::queue<int, 10>::create();
queue3 = std::move(queue2);

// ❌ Copy construction (deleted)
// auto queue4 = queue2;  // Compile error

// ❌ Copy assignment (deleted)
// queue3 = queue2;  // Compile error
```

### Error Handling Integration

Use `cjf` error handling macros with `std::expected`:

```cpp
auto init_queue() -> std::expected<cjf::freertos::queue<int, 10>, esp_err_t> {
    auto result = cjf::freertos::queue<int, 10>::create();
    RETURN_ON_UNEXPECTED(result, TAG, "Failed to create queue");

    // Can now use result as a value
    return result;
}
```

### Service Pattern

Store wrappers as members in service classes:

```cpp
class my_service {
public:
    static std::expected<my_service, esp_err_t> init() {
        my_service service;

        // Initialize primitives
        auto queue_result = cjf::freertos::queue<int, 10>::create();
        RETURN_ON_UNEXPECTED(queue_result, TAG, "Queue creation failed");
        service.queue_ = std::move(*queue_result);

        auto mutex_result = cjf::freertos::mutex::create();
        RETURN_ON_UNEXPECTED(mutex_result, TAG, "Mutex creation failed");
        service.mutex_ = std::move(*mutex_result);

        // Create task that references member variables
        auto task_result = cjf::freertos::task_with_controller<4096>::create(
            [&service]() noexcept {
                service.task_function();
            },
            "my_service",
            5
        );
        RETURN_ON_UNEXPECTED(task_result, TAG, "Task creation failed");
        service.task_ = std::move(*task_result);

        return service;
    }

private:
    void task_function() noexcept {
        task_.controller.wait_for_start();

        while (task_.controller.should_run()) {
            int value;
            if (queue_.receive(value, std::chrono::milliseconds(100)) == ESP_OK) {
                cjf::freertos::lock_guard lock(mutex_);
                // Process value
            }
        }
    }

    cjf::freertos::task_with_controller<4096> task_;
    cjf::freertos::queue<int, 10> queue_;
    cjf::freertos::mutex mutex_;
};
```

## Comparison to Dynamic Allocation

| Feature | Static Wrappers | Dynamic FreeRTOS |
|---------|----------------|------------------|
| Heap usage | Zero | Per-primitive allocation |
| Fragmentation risk | None | Accumulates over time |
| Creation failure | Compile-time storage | Runtime heap exhaustion |
| Resource cleanup | Automatic (RAII) | Manual `vTaskDelete()` etc |
| Type safety | Compile-time checks | Runtime void* casting |
| Stack overflow | Compile-time size | Runtime detection only |

## Implementation Notes

### Why `std::function` instead of `std::move_only_function`?

ESP-IDF 5.5.1 with GCC 13.x does not provide `std::move_only_function`. The wrappers use `std::function<void()>` as a fallback, with `requires` clauses ensuring compile-time noexcept enforcement:

```cpp
template<typename Func>
requires std::is_invocable_r_v<void, Func> && std::is_nothrow_invocable_v<Func>
static std::expected<task, esp_err_t> create(Func&& func, ...);
```

This provides equivalent compile-time safety without the C++23 library feature.

### Stack Size Units

FreeRTOS stack sizes are in **words** (architecture-dependent), not bytes:

```cpp
// ESP32-S3 (32-bit architecture)
task<4096>::create(...)  // 4096 words = 16384 bytes
```

### Thread Safety

All wrappers are **thread-safe** for concurrent access from different tasks, as guaranteed by FreeRTOS API thread safety. However:

- **Mutex**: Must unlock from same task that locked
- **Move operations**: Not thread-safe (single owner assumption)

## Testing

See [test/test_freertos_wrappers.cpp](test/test_freertos_wrappers.cpp) for comprehensive examples.

## References

- [FreeRTOS Static Allocation](https://www.freertos.org/Static_Vs_Dynamic_Memory_Allocation.html)
- [ESP-IDF FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)
- [`cjf::task_controller`](include/cjf/task_controller.h) - Cooperative task lifecycle management
