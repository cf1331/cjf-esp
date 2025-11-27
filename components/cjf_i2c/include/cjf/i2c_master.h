#ifndef C6A4D56D_89CA_4460_A867_0F039BA17D36
#define C6A4D56D_89CA_4460_A867_0F039BA17D36

#include <cjf/error_handling.h>
#include <cstdint>
#include <driver/i2c_master.h>
#include <expected>
#include <memory>

namespace cjf
{
  struct i2c_master_deleter
  {
    void operator()(i2c_master_bus_handle_t bus) const noexcept;
  };

  class i2c_master
  {
  public:
    // Delete copy operations
    i2c_master(const i2c_master&) = delete;
    i2c_master& operator=(const i2c_master&) = delete;

    // Default move operations - explicitly noexcept
    i2c_master(i2c_master&&) noexcept = default;
    i2c_master& operator=(i2c_master&&) noexcept = default;

    static std::expected<i2c_master, esp_err_t> init(
        const i2c_port_num_t port,
        const gpio_num_t sda_io,
        const gpio_num_t scl_io);

    static std::expected<i2c_master, esp_err_t> init(
        const i2c_master_bus_config_t &config);

    operator i2c_master_bus_handle_t() const noexcept;
    i2c_port_num_t port() const noexcept;
    esp_err_t probe(const uint16_t address, int32_t timeout_ms) const;

  private:
    std::unique_ptr<i2c_master_bus_t, i2c_master_deleter> handle_;
    i2c_port_num_t port_;

    explicit i2c_master(i2c_master_bus_handle_t handle, i2c_port_num_t port) noexcept;
  };

} // namespace cjf

#endif /* C6A4D56D_89CA_4460_A867_0F039BA17D36 */
