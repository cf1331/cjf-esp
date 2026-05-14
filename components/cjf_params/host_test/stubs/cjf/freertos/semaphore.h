#pragma once
// Host stub: replaces cjf/freertos/semaphore.h for non-ESP32 builds.
// Implements cjf::freertos::mutex and lock_guard using std::mutex.
#include <mutex>

namespace cjf::freertos
{

  class mutex
  {
  public:
    mutex() = default;
    ~mutex() = default;

    mutex(const mutex &) = delete;
    mutex &operator=(const mutex &) = delete;
    mutex(mutex &&) = delete;
    mutex &operator=(mutex &&) = delete;

    void lock()   { mtx_.lock(); }
    void unlock() { mtx_.unlock(); }

  private:
    std::mutex mtx_;
  };

  class lock_guard
  {
  public:
    explicit lock_guard(mutex &m) : mutex_(m) { mutex_.lock(); }
    ~lock_guard() { mutex_.unlock(); }

    lock_guard(const lock_guard &) = delete;
    lock_guard &operator=(const lock_guard &) = delete;

    bool locked() const { return true; }
    explicit operator bool() const { return true; }

  private:
    mutex &mutex_;
  };

} // namespace cjf::freertos
