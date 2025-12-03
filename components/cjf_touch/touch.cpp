#include "cjf/touch.h"
#include <driver/touch_sens.h>

namespace cjf
{
  static const char *TAG = "cjf:touch";

  std::expected<touch_controller, esp_err_t> touch_controller::init(const config_type &config) noexcept
  {
    touch_sensor_handle_t handle_raw;
    esp_err_t err = touch_sensor_new_controller(&config, &handle_raw);
    handle_type handle(handle_raw);
    RETURN_UNEXPECTED_ON_ERROR(err, TAG);
    return touch_controller(std::move(handle));
  }

  esp_err_t touch_controller::reconfigure(const config_type &config) noexcept
  {
    return touch_sensor_reconfig_controller(*this, &config);
  }

  touch_controller::operator touch_sensor_handle_t() const noexcept
  {
    return handle_.get();
  }

  void touch_controller::deleter::operator()(touch_sensor_handle_t handle) const
  {
    if (handle)
    {
      LOG_IF_ERROR(touch_sensor_del_controller(handle), TAG);
    }
  }

  touch_controller::touch_controller(handle_type handle) noexcept
      : handle_(std::move(handle)) {}

  std::expected<touch_channel, esp_err_t> touch_channel::init(
      touch_controller &controller,
      const int channel_id,
      const config_type &config) noexcept
  {
    touch_channel_handle_t handle_raw;
    esp_err_t err = touch_sensor_new_channel(
        static_cast<touch_sensor_handle_t>(controller),
        channel_id,
        &config,
        &handle_raw);
    handle_type handle(handle_raw);
    RETURN_UNEXPECTED_ON_ERROR(err, TAG);
    return touch_channel(std::move(handle), channel_id);
  }

  int32_t touch_channel::channel_id() const noexcept
  {
    return channel_id_;
  }

  std::expected<uint32_t, esp_err_t> touch_channel::read_benchmark() noexcept
  {
    uint32_t benchmark = 0;
    RETURN_UNEXPECTED_ON_ERROR(touch_channel_read_data(*this, TOUCH_CHAN_DATA_TYPE_BENCHMARK, &benchmark), TAG);
    return benchmark;
  }

  std::expected<uint32_t, esp_err_t> touch_channel::read_raw() noexcept
  {
    uint32_t raw = 0;
    RETURN_UNEXPECTED_ON_ERROR(touch_channel_read_data(*this, TOUCH_CHAN_DATA_TYPE_RAW, &raw), TAG);
    return raw;
  }

  std::expected<uint32_t, esp_err_t> touch_channel::read_smooth() noexcept
  {
    uint32_t smooth = 0;
    RETURN_UNEXPECTED_ON_ERROR(touch_channel_read_data(*this, TOUCH_CHAN_DATA_TYPE_SMOOTH, &smooth), TAG);
    return smooth;
  }

  esp_err_t touch_channel::reconfigure(const config_type &config) noexcept
  {
    return touch_sensor_reconfig_channel(*this, &config);
  }

  touch_channel::operator touch_channel_handle_t() const noexcept
  {
    return handle_.get();
  }

  void touch_channel::deleter::operator()(touch_channel_handle_t handle) const
  {
    if (handle)
    {
      LOG_IF_ERROR(touch_sensor_del_channel(handle), TAG);
    }
  }

  touch_channel::touch_channel(handle_type handle, int32_t channel_id) noexcept
      : handle_(std::move(handle)), channel_id_(channel_id) {}

} // namespace cjf