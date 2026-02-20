#ifndef D3C007AF_E7C7_4918_B958_8528AED49218
#define D3C007AF_E7C7_4918_B958_8528AED49218

#include <driver/i2c_types.h>
#include <memory>

template <typename HandleType, typename DeleterType>
class i2c_bus
{
public:
  // Move only - this is a RAII wrapper around an I2C bus handle (master or slave).
  // Only one instance should own that handle at a time. When the owning instance is
  // destroyed, the handle will be automatically cleaned up by the deleter.
  i2c_bus(const i2c_bus &) = delete;
  i2c_bus &operator=(const i2c_bus &) = delete;
  i2c_bus(i2c_bus &&) noexcept = default;
  i2c_bus &operator=(i2c_bus &&) noexcept = default;

  operator HandleType() const noexcept { return handle_.get(); }
  i2c_port_num_t port() const noexcept { return port_; }

protected:
  explicit i2c_bus(HandleType handle, i2c_port_num_t port) noexcept
      : handle_(std::move(handle)), port_(port) {}

private:
  std::unique_ptr<std::remove_pointer_t<HandleType>, DeleterType> handle_;
  i2c_port_num_t port_;
};

#endif /* D3C007AF_E7C7_4918_B958_8528AED49218 */
