#ifndef A86CF28F_EFBE_42F2_A115_BD8C4D242848
#define A86CF28F_EFBE_42F2_A115_BD8C4D242848

#include <chrono>
#include <cjf/error_handling.h>
#include <driver/touch_sens.h>
#include <expected>
#include <memory>
#include <tuple>
#include <vector>

namespace cjf
{
  class touch_controller
  {
  public:
    using config_type = touch_sensor_config_t;

    static std::expected<touch_controller, esp_err_t> init(
        const config_type &config) noexcept;

    esp_err_t reconfigure(const config_type &config) noexcept;

    operator touch_sensor_handle_t() const noexcept;

  private:
    struct deleter
    {
      void operator()(touch_sensor_handle_t handle) const;
    };

    using handle_type = std::unique_ptr<touch_sensor_s, deleter>;
    handle_type handle_;

    touch_controller(handle_type handle) noexcept;
  };

  class touch_channel
  {
  public:
    using config_type = touch_channel_config_t;

    static std::expected<touch_channel, esp_err_t> init(
        touch_controller &controller,
        const int channel_id,
        const config_type &config) noexcept;

    esp_err_t reconfigure(const config_type &config) noexcept;

    operator touch_channel_handle_t() const noexcept;

  private:
    struct deleter
    {
      void operator()(touch_channel_handle_t handle) const;
    };

    using handle_type = std::unique_ptr<touch_channel_s, deleter>;
    handle_type handle_;

    touch_channel(handle_type handle) noexcept;
  };

} // namespace cjf

#endif // A86CF28F_EFBE_42F2_A115_BD8C4D242848
