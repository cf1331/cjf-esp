#include "cjf/web_server/mjpeg_middleware.h"

namespace cjf
{

  static const char *TAG = "cjf:mjpeg_middleware";

  mjpeg_middleware::mjpeg_middleware(const config_type &config)
      : stream_({.boundary = config.boundary,
                 .part_content_type = config.part_content_type,
                 .on_stream_start = config.on_stream_start,
                 .on_stream_end = config.on_stream_end,
                 .queue_depth = config.queue_depth})
  {
    is_async = true;
    handler = [this](httpd_req_t *req, web_server::next_func next) -> esp_err_t
    {
      return stream_.handler(req, next);
    };
  }

  void mjpeg_middleware::end(TickType_t ticks_to_wait)
  {
    stream_.end(ticks_to_wait);
  }

  esp_err_t mjpeg_middleware::write(std::shared_ptr<camera_fb_t> frame, TickType_t ticks_to_wait)
  {
    if (!frame)
    {
      ESP_LOGE(TAG, "Frame pointer is null");
      return ESP_ERR_INVALID_ARG;
    }
    // Use aliasing constructor to create a shared_ptr<const void> that points to frame->buf
    // but shares ownership with the camera_fb_t, ensuring the frame is freed after transmission
    std::shared_ptr<const void> data(frame, frame->buf);
    return stream_.write(
        data,
        frame->len,
        ticks_to_wait);
  }

} // namespace cjf
