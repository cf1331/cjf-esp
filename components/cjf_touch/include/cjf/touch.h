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

    touch_controller(const touch_controller &) = delete;
    touch_controller &operator=(const touch_controller &) = delete;
    touch_controller(touch_controller &&) noexcept = default;
    touch_controller &operator=(touch_controller &&) noexcept = default;

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

    touch_channel(const touch_channel &) = delete;
    touch_channel &operator=(const touch_channel &) = delete;
    touch_channel(touch_channel &&) noexcept = default;
    touch_channel &operator=(touch_channel &&) noexcept = default;

    int32_t channel_id() const noexcept;

    std::expected<uint32_t, esp_err_t> read_benchmark() noexcept;
    std::expected<uint32_t, esp_err_t> read_raw() noexcept;
    std::expected<uint32_t, esp_err_t> read_smooth() noexcept;

    esp_err_t reconfigure(const config_type &config) noexcept;

    operator touch_channel_handle_t() const noexcept;

  private:
    struct deleter
    {
      void operator()(touch_channel_handle_t handle) const;
    };

    using handle_type = std::unique_ptr<touch_channel_s, deleter>;
    handle_type handle_;
    int32_t channel_id_; // TODO: invalidate the channel_id when the channel is moved

    touch_channel(handle_type handle, int32_t channel_id) noexcept;
  };

} // namespace cjf

#endif // A86CF28F_EFBE_42F2_A115_BD8C4D242848
