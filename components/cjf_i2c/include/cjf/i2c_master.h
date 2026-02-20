#ifndef C6A4D56D_89CA_4460_A867_0F039BA17D36
#define C6A4D56D_89CA_4460_A867_0F039BA17D36

#include "cjf/i2c_bus.h"
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

  class i2c_master : public i2c_bus<i2c_master_bus_handle_t, i2c_master_deleter>
  {
  public:
    static constexpr const i2c_master_bus_config_t DEFAULT_CONFIG = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_NC,
        .scl_io_num = GPIO_NUM_NC,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 0,
            .allow_pd = 0}};

    // Move only - this is a RAII wrapper around an I2C master bus handle.
    // Only one instance should own that handle at a time. When the owning instance is
    // destroyed, the handle will be automatically cleaned up by the deleter.
    i2c_master(const i2c_master &) = delete;
    i2c_master &operator=(const i2c_master &) = delete;
    i2c_master(i2c_master &&) noexcept = default;
    i2c_master &operator=(i2c_master &&) noexcept = default;

    static std::expected<i2c_master, esp_err_t> init(
        const i2c_port_num_t port,
        const gpio_num_t sda_io,
        const gpio_num_t scl_io);

    static std::expected<i2c_master, esp_err_t> init(
        const i2c_master_bus_config_t &config);

    esp_err_t probe(const uint16_t address, int32_t timeout_ms) const;

  private:
    using i2c_bus::i2c_bus; // Inherit constructors
  };

} // namespace cjf

#endif /* C6A4D56D_89CA_4460_A867_0F039BA17D36 */
