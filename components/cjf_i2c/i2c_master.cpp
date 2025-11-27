#include "cjf/i2c_master.h"
#include <cjf/error_handling.h>

namespace cjf
{
  const char *CJF_I2C_MASTER = "cjf:i2c_master";

  void i2c_master_deleter::operator()(i2c_master_bus_handle_t bus) const noexcept
  {
    LOG_IF_ERROR(i2c_del_master_bus(bus), CJF_I2C_MASTER);
  }

  std::expected<i2c_master, esp_err_t> i2c_master::init(
      const i2c_port_num_t port,
      const gpio_num_t sda_io,
      const gpio_num_t scl_io)
  {
    i2c_master_bus_config_t config = {
        .i2c_port = port,
        .sda_io_num = sda_io,
        .scl_io_num = scl_io,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
          .enable_internal_pullup = 1,
          .allow_pd = 1
        }
    };
    return init(config);
  };

  std::expected<i2c_master, esp_err_t> i2c_master::init(
      const i2c_master_bus_config_t &config)
  {
    i2c_master_bus_handle_t handle = nullptr;
    RETURN_UNEXPECTED_ON_ERROR(i2c_new_master_bus(&config, &handle), CJF_I2C_MASTER);
    return i2c_master(handle, config.i2c_port);
  }

  i2c_master::i2c_master(i2c_master_bus_handle_t handle, i2c_port_num_t port) noexcept
      : handle_(handle), port_(port) {}

  i2c_master::operator i2c_master_bus_handle_t() const noexcept
  {
    return handle_.get();
  }

  i2c_port_num_t i2c_master::port() const noexcept
  {
    return port_;
  }

  esp_err_t i2c_master::probe(const uint16_t address, int32_t timeout_ms) const
  {
    return i2c_master_probe(*this, address, timeout_ms);
  }

} // namespace cjf
