#ifndef B18C78FD_590F_4F0A_8588_25DD7C9B26F8
#define B18C78FD_590F_4F0A_8588_25DD7C9B26F8

#include <cassert>
#include <cstddef>
#include <freertos/ringbuf.h>
#include <memory>

namespace cjf::freertos
{

  template <size_t Size>
  class ring_buffer
  {
    static_assert(Size % 4 == 0, "ring_buffer Size must be 32-bit aligned for no-split/allow-split buffer types");

  public:
    struct item_deleter
    {
      RingbufHandle_t handle = nullptr;
      void operator()(void *item) const noexcept
      {
        if (handle != nullptr && item != nullptr)
        {
          vRingbufferReturnItem(handle, item);
        }
      }
    };

    struct item_type
    {
      std::unique_ptr<void, item_deleter> data = {nullptr, {nullptr}};
      size_t size = 0;

      explicit operator bool() const noexcept
      {
        return data != nullptr;
      }
    };

    struct split_item_type
    {
      item_type head;
      item_type tail;

      operator bool() const noexcept
      {
        return head.data != nullptr;
      }

      bool is_split() const noexcept
      {
        return head.data != nullptr && tail.data != nullptr;
      }
    };

    ring_buffer(RingbufferType_t buffer_type) noexcept
        : handle_(xRingbufferCreateStatic(Size, buffer_type, buffer_, &control_block_))
    {
    }

    ~ring_buffer() noexcept
    {
      if (handle_ != nullptr)
      {
        vRingbufferDelete(handle_);
      }
    }

    // Non-copyable and non-movable - buffer storage address is registered with FreeRTOS and must not change.
    ring_buffer(const ring_buffer &) = delete;
    ring_buffer &operator=(const ring_buffer &) = delete;
    ring_buffer(ring_buffer &&) = delete;
    ring_buffer &operator=(ring_buffer &&) = delete;

    template <typename T>
    std::unique_ptr<T, item_deleter> receive(TickType_t ticks_to_wait) noexcept
    {
      size_t item_size;
      void *raw_item = xRingbufferReceive(handle_, &item_size, ticks_to_wait);
      if (raw_item && item_size != sizeof(T))
      {
        // Item size mismatch, return item and fail
        vRingbufferReturnItem(handle_, raw_item);
        return {nullptr, {handle_}};
      }
      return std::unique_ptr<T, item_deleter>(static_cast<T *>(raw_item), {handle_});
    }

    item_type receive(TickType_t ticks_to_wait) noexcept
    {
      item_type item;
      void *raw_item = xRingbufferReceive(handle_, &item.size, ticks_to_wait);
      if (raw_item != nullptr)
      {
        item.data = std::unique_ptr<void, item_deleter>(raw_item, {handle_});
      }
      return item;
    }

    split_item_type receive_split(TickType_t ticks_to_wait) noexcept
    {
      split_item_type item;
      void *head_ptr = nullptr;
      void *tail_ptr = nullptr;
      xRingbufferReceiveSplit(handle_, &head_ptr, &tail_ptr, &item.head.size, &item.tail.size, ticks_to_wait);
      if (head_ptr != nullptr)
      {
        item.head.data = std::unique_ptr<void, item_deleter>(head_ptr, {handle_});
      }
      if (tail_ptr != nullptr)
      {
        item.tail.data = std::unique_ptr<void, item_deleter>(tail_ptr, {handle_});
      }
      return item;
    }

    item_type receive_up_to(TickType_t ticks_to_wait, size_t max_size) noexcept
    {
      item_type item;
      void *raw_item = xRingbufferReceiveUpTo(handle_, &item.size, ticks_to_wait, max_size);
      if (raw_item != nullptr)
      {
        item.data = std::unique_ptr<void, item_deleter>(raw_item, {handle_});
      }
      return item;
    }

    template <typename T>
    bool send(const T &item, TickType_t ticks_to_wait) noexcept
    {
      return send(static_cast<const void *>(&item), sizeof(T), ticks_to_wait);
    }

    bool send(const void *item, const size_t item_size, const TickType_t ticks_to_wait) noexcept
    {
      return xRingbufferSend(handle_, item, item_size, ticks_to_wait) == pdTRUE;
    }

    template <typename T>
    T *send_acquire(TickType_t ticks_to_wait) noexcept
    {
      return static_cast<T *>(send_acquire(sizeof(T), ticks_to_wait));
    }

    void *send_acquire(size_t item_size, TickType_t ticks_to_wait) noexcept
    {
      void *item = nullptr;
      xRingbufferSendAcquire(handle_, &item, item_size, ticks_to_wait);
      return item;
    }

    template <typename T>
    bool send_complete(T *item) noexcept
    {
      return send_complete(static_cast<void *>(item));
    }

    bool send_complete(void *item) noexcept
    {
      return xRingbufferSendComplete(handle_, item) == pdTRUE;
    }

  private:
    StaticRingbuffer_t control_block_;
    uint8_t buffer_[Size];
    RingbufHandle_t handle_ = nullptr;
  };

} // namespace cjf::freertos

#endif /* B18C78FD_590F_4F0A_8588_25DD7C9B26F8 */
