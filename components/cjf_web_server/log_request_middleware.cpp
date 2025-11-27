#include "cjf/web_server/log_request_middleware.h"
#include <esp_log.h>
#include <esp_timer.h>

namespace cjf
{
  static const char *TAG = "cjf:web_server:log_request_middleware";

  log_request_middleware::log_request_middleware()
  {
    name = "log_request";
    handler = [](httpd_req_t *req, web_server::next_func next)
    {
      int64_t start_time = esp_timer_get_time();
      const char* method = http_method_str(static_cast<httpd_method_t>(req->method));
      ESP_LOGI(TAG, "Received %s %s", method, req->uri);
      esp_err_t ret = next();
      int64_t duration_ms = (esp_timer_get_time() - start_time) / 1000;
      ESP_LOGI(TAG, "Finished %s %s (%lld ms)", method, req->uri, duration_ms);
      return ret;
    };
  }
}