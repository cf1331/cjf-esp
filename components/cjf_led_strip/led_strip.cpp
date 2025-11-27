#include "cjf/led_strip.h"
#include <cjf/error_handling.h>

static const char *CJF_LED_STRIP = "cjf::led_strip";

namespace cjf
{
  const led_strip_rmt_config_t led_strip::rmt_config_default = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 10 * 1000 * 1000, // 10MHz
      .mem_block_symbols = 0,            // Let driver choose
      .flags = {.with_dma = 0}};

  void led_strip_deleter::operator()(led_strip_handle_t led_strip) const noexcept
  {
    LOG_IF_ERROR(led_strip_del(led_strip), CJF_LED_STRIP);
  }

  std::expected<led_strip, esp_err_t> led_strip::init(
      const led_strip_rmt_config_t &rmt_config,
      const led_strip_config_t &strip_config)
  {
    led_strip_handle_t handle = nullptr;
    RETURN_UNEXPECTED_ON_ERROR(led_strip_new_rmt_device(&strip_config, &rmt_config, &handle), CJF_LED_STRIP);
    return led_strip(handle);
  }

  led_strip::led_strip(led_strip_handle_t handle) noexcept
      : handle_(handle) {}

  led_strip::operator led_strip_handle_t() const noexcept
  {
    return handle_.get();
  }

  esp_err_t led_strip::clear() const
  {
    return led_strip_clear(handle_.get());
  }

  esp_err_t led_strip::refresh() const
  {
    return led_strip_refresh(handle_.get());
  }

  esp_err_t led_strip::set_pixel_hsv(size_t pixel_num, uint16_t h, uint8_t s, uint8_t v) const
  {
    return led_strip_set_pixel_hsv(handle_.get(), pixel_num, h, s, v);
  }

  esp_err_t led_strip::set_pixel_rgb(size_t pixel_num, uint8_t r, uint8_t g, uint8_t b) const
  {
    return led_strip_set_pixel(handle_.get(), pixel_num, r, g, b);
  }

  esp_err_t led_strip::set_pixel_rgbw(size_t pixel_num, uint8_t r, uint8_t g, uint8_t b, uint8_t w) const
  {
    return led_strip_set_pixel_rgbw(handle_.get(), pixel_num, r, g, b, w);
  }

} // namespace cjf
