#include "cjf/i2c_device.h"
#include <cjf/error_handling.h>

namespace cjf
{
  static const char *TAG = "cjf:i2c_device";

  void i2c_device::handle_deleter::operator()(i2c_master_dev_handle_t device) const noexcept
  {
    LOG_IF_ERROR(i2c_master_bus_rm_device(device), TAG);
  }

  std::expected<i2c_device::handle_type, esp_err_t> i2c_device::add_to_bus(
      const i2c_master &master,
      const i2c_device_config_t &config)
  {
    i2c_master_dev_handle_t handle = nullptr;
    RETURN_UNEXPECTED_ON_ERROR(i2c_master_bus_add_device(master, &config, &handle), TAG);
    return handle_type(handle, handle_deleter{});
  }

  i2c_device::i2c_device(const i2c_master &master, uint16_t address, handle_type&& handle) noexcept
      : master_(&master), address_(address), handle_(std::move(handle)) {}

  i2c_device::i2c_device(const i2c_master &master, handle_type &&handle, i2c_device_config_t config) noexcept
      : master_(&master), address_(config.device_address), handle_(std::move(handle)), config_(config) {}

  bool i2c_device::is_connected() const noexcept
  {
    return handle_ != nullptr;
  }

  void i2c_device::disconnect() noexcept
  {
    handle_.reset();
    master_ = nullptr;
  }

  esp_err_t i2c_device::connect(const i2c_master &master) noexcept
  {
    auto handle = add_to_bus(master, config_);
    if (!handle)
    {
      ESP_LOGE(CJF_I2C_DEVICE, "Failed to connect device 0x%02x: %s", address_, esp_err_to_name(handle.error()));
      return handle.error();
    }
    handle_ = std::move(*handle);
    master_ = &master;
    return ESP_OK;
  }

  i2c_device::operator i2c_master_dev_handle_t() const noexcept
  {
    return handle_.get();
  }

  uint16_t i2c_device::address() const noexcept
  {
    return address_;
  }

  esp_err_t i2c_device::probe(int32_t timeout_ms) const
  {
    if (!is_connected())
      return ESP_ERR_INVALID_STATE;
    return master_->probe(address(), timeout_ms);
  }

  esp_err_t i2c_device::read(uint8_t *buffer, const size_t buffer_size, const int32_t timeout_ms) const noexcept
  {
    if (!is_connected())
      return ESP_ERR_INVALID_STATE;
    return i2c_master_receive(*this, buffer, buffer_size, timeout_ms);
  }

  esp_err_t i2c_device::write(const uint8_t *buf, const size_t size, const int32_t timeout_ms) const noexcept
  {
    if (!is_connected())
      return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit(*this, buf, size, timeout_ms);
  }

} // namespace cjf