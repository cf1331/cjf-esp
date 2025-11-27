#ifndef BE5FDD17_2215_4749_9B1A_743CD4D392E8
#define BE5FDD17_2215_4749_9B1A_743CD4D392E8

#include <expected>
#include <esp_err.h>
#include <led_strip.h>
#include <memory>

namespace cjf
{
  struct led_strip_deleter
  {
    void operator()(led_strip_handle_t led_strip) const noexcept;
  };

  class led_strip
  {
  public:
    static const led_strip_rmt_config_t rmt_config_default;

    static std::expected<led_strip, esp_err_t> init(
        const led_strip_rmt_config_t &rmt_config,
        const led_strip_config_t &strip_config);

    // Implicit conversion to led_strip_handle_t to allow use with esp-idf led_strip API
    operator led_strip_handle_t() const noexcept;

    esp_err_t clear() const;
    esp_err_t refresh() const;
    esp_err_t set_pixel_hsv(size_t pixel_num, uint16_t h, uint8_t s, uint8_t v) const;
    esp_err_t set_pixel_rgb(size_t pixel_num, uint8_t r, uint8_t g, uint8_t b) const;
    esp_err_t set_pixel_rgbw(size_t pixel_num, uint8_t r, uint8_t g, uint8_t b, uint8_t w) const;

  private:
    std::unique_ptr<led_strip_t, led_strip_deleter> handle_;

    explicit led_strip(led_strip_handle_t handle) noexcept;
  };

} // namespace cjf

#endif /* BE5FDD17_2215_4749_9B1A_743CD4D392E8 */
