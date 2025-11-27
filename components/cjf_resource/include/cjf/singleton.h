#ifndef F6912815_22B7_4A48_89BA_16B35EF75BEC
#define F6912815_22B7_4A48_89BA_16B35EF75BEC

#include <memory>
#include <utility>

namespace cjf
{

  template <typename T>
  class singleton
  {
  private:
    std::unique_ptr<T>* ptr;

  public:
    explicit singleton(std::unique_ptr<T>& p) : ptr(&p) {}

    // Copy constructor - deleted (singletons shouldn't be copied)
    singleton(const singleton&) = delete;
    singleton& operator=(const singleton&) = delete;

    // Move constructor and assignment
    singleton(singleton&& other) noexcept
      : ptr(std::exchange(other.ptr, nullptr)) {}

    singleton& operator=(singleton&& other) noexcept {
      if (this != &other) {
        ptr = std::exchange(other.ptr, nullptr);
      }
      return *this;
    }

    T *operator->() { return ptr ? ptr->get() : nullptr; }
    const T *operator->() const { return ptr ? ptr->get() : nullptr; }

    T &operator*() { return **ptr; }
    const T &operator*() const { return **ptr; }

    // Optionally, expose get() if needed
    T *get() { return ptr ? ptr->get() : nullptr; }
    const T *get() const { return ptr ? ptr->get() : nullptr; }

    // Check if the singleton is valid
    explicit operator bool() const noexcept { return ptr && *ptr; }
  };

} // namespace cjf

#endif // F6912815_22B7_4A48_89BA_16B35EF75BEC
