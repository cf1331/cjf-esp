#include "cjf/sntp.h"
#include <sys/time.h>

ESP_EVENT_DEFINE_BASE(CJF_SNTP_EVENT);

namespace cjf
{
  const char *CJF_SNTP = "cjf:sntp";

  std::expected<sntp_service, esp_err_t> sntp_service::start()
  {
    return start(sntp_service::config<1>{
        .servers = { default_server }});
  }

  void sntp_service::deleter::operator()() const noexcept
  {
    esp_netif_sntp_deinit();
    ESP_LOGI(CJF_SNTP, "SNTP service stopped");
  }

  sntp_service::sntp_service()
  {
    ESP_LOGI(CJF_SNTP, "SNTP service started");
  }

  esp_err_t sntp_service::sync()
  {
    RETURN_ON_ERROR(esp_netif_sntp_start(), CJF_SNTP);
    return ESP_OK;
  }

  esp_err_t sntp_service::sync(TickType_t timeout)
  {
    RETURN_ON_ERROR(esp_netif_sntp_start(), CJF_SNTP);
    RETURN_ON_ERROR(esp_netif_sntp_sync_wait(timeout), CJF_SNTP);
    return ESP_OK;
  }

  void sntp_service::sync_callback_(timeval *tv)
  {
    char strftime_buf[64];
    struct tm timeinfo;
    localtime_r(&tv->tv_sec, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c %Z", &timeinfo);
    ESP_LOGI(CJF_SNTP, "%s", strftime_buf);
    esp_event_post(CJF_SNTP_EVENT, CJF_SNTP_EVENT_SYNCED, tv, sizeof(*tv), portMAX_DELAY);
  }

}