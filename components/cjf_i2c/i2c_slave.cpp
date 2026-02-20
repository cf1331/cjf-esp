#include "cjf/i2c_slave.h"
#include <cjf/error_handling.h>

namespace cjf
{
  static const char *TAG = "cjf:i2c_slave";

  void i2c_slave_deleter::operator()(i2c_slave_dev_handle_t bus) const noexcept
  {
    LOG_IF_ERROR(i2c_del_slave_device(bus), TAG);
  }

  std::expected<i2c_slave, esp_err_t> i2c_slave::init(
      const i2c_port_num_t port,
      const gpio_num_t sda_io,
      const gpio_num_t scl_io)
  {
    i2c_slave_config_t config = DEFAULT_CONFIG;
    config.i2c_port = port;
    config.sda_io_num = sda_io;
    config.scl_io_num = scl_io;
    return init(config);
  }

  std::expected<i2c_slave, esp_err_t> i2c_slave::init(
      const i2c_slave_config_t &config)
  {
    i2c_slave_dev_handle_t handle = nullptr;
    RETURN_UNEXPECTED_ON_ERROR(i2c_new_slave_device(&config, &handle), TAG);
    return i2c_slave(handle, config.i2c_port);
  }

} // namespace cjf
