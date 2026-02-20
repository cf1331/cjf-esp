#ifndef E585C4A4_AA7B_4EE1_BA4A_60D388713CB5
#define E585C4A4_AA7B_4EE1_BA4A_60D388713CB5

#include "cjf/i2c_bus.h"
#include <driver/i2c_slave.h>
#include <expected>
#include <memory>

namespace cjf
{
  struct i2c_slave_deleter
  {
    void operator()(i2c_slave_dev_handle_t bus) const noexcept;
  };

  class i2c_slave : public i2c_bus<i2c_slave_dev_handle_t, i2c_slave_deleter>
  {
  public:
    static constexpr const i2c_slave_config_t DEFAULT_CONFIG = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_NC,
        .scl_io_num = GPIO_NUM_NC,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .send_buf_depth = 0,
        .receive_buf_depth = 0,
        .slave_addr = 0,
        .addr_bit_len = I2C_ADDR_BIT_LEN_7,
        .intr_priority = 0,
        .flags = {
            .allow_pd = 0,
            .enable_internal_pullup = 0,
#if SOC_I2C_SLAVE_SUPPORT_BROADCAST
            .broadcast_en = 0,
#endif
        }};

    // Move only - this is a RAII wrapper around an I2C slave bus handle.
    // Only one instance should own that handle at a time. When the owning instance is
    // destroyed, the handle will be automatically cleaned up by the deleter.
    i2c_slave(const i2c_slave &) = delete;
    i2c_slave &operator=(const i2c_slave &) = delete;
    i2c_slave(i2c_slave &&) noexcept = default;
    i2c_slave &operator=(i2c_slave &&) noexcept = default;

    static std::expected<i2c_slave, esp_err_t> init(
        const i2c_port_num_t port,
        const gpio_num_t sda_io,
        const gpio_num_t scl_io);

    static std::expected<i2c_slave, esp_err_t> init(
        const i2c_slave_config_t &config);

  private:
    using i2c_bus::i2c_bus; // Inherit constructors
  };

} // namespace cjf

#endif /* E585C4A4_AA7B_4EE1_BA4A_60D388713CB5 */
