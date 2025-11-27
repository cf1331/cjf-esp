#ifndef BFD0CE43_9F6B_4240_96C7_D4657EC70BF4
#define BFD0CE43_9F6B_4240_96C7_D4657EC70BF4

namespace cjf
{
  template <typename HandleType>
  class resource_with_handle
  {
  public:
    // Delete copy operations to ensure move-only semantics
    resource_with_handle(const resource_with_handle &) = delete;
    resource_with_handle &operator=(const resource_with_handle &) = delete;

    // Move constructor - transfers ownership of the handle
    resource_with_handle(resource_with_handle &&other) noexcept
        : handle_(other.handle_)
    {
      // Invalidate the source handle
      other.handle_ = HandleType{};
    }

    // Move assignment - transfers ownership of the handle
    resource_with_handle &operator=(resource_with_handle &&other) noexcept
    {
      if (this != &other)
      {
        handle_ = other.handle_;
        // Invalidate the source handle
        other.handle_ = HandleType{};
      }
      return *this;
    }

    // Implicit conversion operator to handle type T
    operator HandleType() const noexcept
    {
      return handle_;
    }

    HandleType handle() const noexcept
    {
      return handle_;
    }

  protected:
    HandleType handle_;

    resource_with_handle(HandleType handle) noexcept
        : handle_(handle)
    {
    }

    // Destructor - derrived classes must implement this to free up resources associated
    // with the handle. It is protected in this base classs to ensure that the base class
    // cannot be used directly with smart pointers.
    virtual ~resource_with_handle() = 0;
  };

  // Implementation of the pure virtual destructor
  template <typename HandleType>
  resource_with_handle<HandleType>::~resource_with_handle() {}

} // namespace cjf

#endif // BFD0CE43_9F6B_4240_96C7_D4657EC70BF4
