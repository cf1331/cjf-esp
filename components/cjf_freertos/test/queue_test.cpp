#include <cjf/freertos/queue.h>
#include <esp_log.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace cjf::freertos;

static const char* TAG = "test:queue";

struct test_message {
    uint32_t id;
    float value;
};

TEST_CASE("Queue constructs successfully", "[freertos][queue]")
{
    queue<int, 10> my_queue;

    TEST_ASSERT_TRUE(static_cast<bool>(my_queue));
    TEST_ASSERT_EQUAL(ESP_OK, static_cast<esp_err_t>(my_queue));
    TEST_ASSERT_NOT_NULL(my_queue.handle());
    TEST_ASSERT_EQUAL(0, my_queue.size());
    TEST_ASSERT_TRUE(my_queue.empty());
}

TEST_CASE("Queue send and receive", "[freertos][queue]")
{
    queue<int, 5> my_queue;
    TEST_ASSERT_TRUE(static_cast<bool>(my_queue));

    // Send items
    for (int i = 0; i < 3; i++) {
        esp_err_t err = my_queue.send(i * 10, pdMS_TO_TICKS(100));
        TEST_ASSERT_EQUAL(ESP_OK, err);
    }

    TEST_ASSERT_EQUAL(3, my_queue.size());
    TEST_ASSERT_FALSE(my_queue.empty());

    // Receive items
    for (int i = 0; i < 3; i++) {
        int value;
        esp_err_t err = my_queue.receive(value, pdMS_TO_TICKS(100));
        TEST_ASSERT_EQUAL(ESP_OK, err);
        TEST_ASSERT_EQUAL(i * 10, value);
    }

    TEST_ASSERT_EQUAL(0, my_queue.size());
    TEST_ASSERT_TRUE(my_queue.empty());
}

TEST_CASE("Queue with struct type", "[freertos][queue]")
{
    queue<test_message, 10> my_queue;
    TEST_ASSERT_TRUE(static_cast<bool>(my_queue));

    test_message msg{.id = 42, .value = 3.14f};
    esp_err_t err = my_queue.send(msg, pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);

    test_message received;
    err = my_queue.receive(received, pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(42, received.id);
    TEST_ASSERT_EQUAL_FLOAT(3.14f, received.value);
}

TEST_CASE("Queue full detection", "[freertos][queue]")
{
    queue<int, 3> my_queue;
    TEST_ASSERT_TRUE(static_cast<bool>(my_queue));

    // Fill the queue
    for (int i = 0; i < 3; i++) {
        esp_err_t err = my_queue.send(i, pdMS_TO_TICKS(100));
        TEST_ASSERT_EQUAL(ESP_OK, err);
    }

    TEST_ASSERT_TRUE(my_queue.full());
    TEST_ASSERT_EQUAL(0, my_queue.available());

    // Try to send to full queue (should timeout)
    esp_err_t err = my_queue.send(999, 0);  // No wait
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, err);
}

TEST_CASE("Queue peek without removing", "[freertos][queue]")
{
    queue<int, 5> my_queue;
    TEST_ASSERT_TRUE(static_cast<bool>(my_queue));

    int value = 123;
    esp_err_t err = my_queue.send(value, pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);

    // Peek should not remove item
    int peeked;
    err = my_queue.peek(peeked, pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(123, peeked);
    TEST_ASSERT_EQUAL(1, my_queue.size());  // Still in queue

    // Now receive it
    int received;
    err = my_queue.receive(received, pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(123, received);
    TEST_ASSERT_EQUAL(0, my_queue.size());
}

TEST_CASE("Queue send to front", "[freertos][queue]")
{
    queue<int, 5> my_queue;
    TEST_ASSERT_TRUE(static_cast<bool>(my_queue));

    // Send items normally
    my_queue.send(10, pdMS_TO_TICKS(100));
    my_queue.send(20, pdMS_TO_TICKS(100));

    // Send to front
    esp_err_t err = my_queue.send_to_front(5, pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(ESP_OK, err);

    // Front item should be received first
    int value;
    my_queue.receive(value, pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(5, value);

    my_queue.receive(value, pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(10, value);
}

TEST_CASE("Queue reset", "[freertos][queue]")
{
    queue<int, 5> my_queue;
    TEST_ASSERT_TRUE(static_cast<bool>(my_queue));

    // Add items
    for (int i = 0; i < 3; i++) {
        my_queue.send(i, pdMS_TO_TICKS(100));
    }
    TEST_ASSERT_EQUAL(3, my_queue.size());

    // Reset queue
    esp_err_t err = my_queue.reset();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(0, my_queue.size());
    TEST_ASSERT_TRUE(my_queue.empty());
}

TEST_CASE("Queue chrono timeout", "[freertos][queue]")
{
    using namespace std::chrono_literals;

    queue<int, 5> my_queue;
    TEST_ASSERT_TRUE(static_cast<bool>(my_queue));

    // Send with chrono duration
    esp_err_t err = my_queue.send(42, 100ms);
    TEST_ASSERT_EQUAL(ESP_OK, err);

    // Receive with chrono duration
    int value;
    err = my_queue.receive(value, 100ms);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(42, value);
}
