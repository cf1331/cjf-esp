#ifndef C8027565_C747_416D_86EE_785839AEC252
#define C8027565_C747_416D_86EE_785839AEC252

#include <cjf/web_server.h>
#include <cjf/web_server/multipart_stream_middleware.h>
#include <esp_camera.h>

namespace cjf
{
  class mjpeg_middleware : public web_server::middleware
  {
  public:
    static constexpr const char *name = "mjpeg_middleware";

    using stream_start_func = multipart_stream::stream_start_func;
    using stream_end_func = multipart_stream::stream_end_func;

    struct config_type
    {
      const char *boundary = "FRAME";
      const char *part_content_type = "image/jpeg";
      stream_start_func on_stream_start = NULL;
      stream_end_func on_stream_end = NULL;
      size_t queue_depth = 3;
    };

    mjpeg_middleware(const config_type &config);

    void end(TickType_t ticks_to_wait = portMAX_DELAY);
    esp_err_t write(std::shared_ptr<camera_fb_t> frame, TickType_t ticks_to_wait = portMAX_DELAY);

  private:
    multipart_stream stream_;
  };
} // namespace cjf

#endif /* C8027565_C747_416D_86EE_785839AEC252 */
