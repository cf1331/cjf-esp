#include "cjf/wifi/wifi_mode_soft_ap.h"
#include <cjf/error_handling.h>
#include <cstring>
#include <esp_log.h>
#include <esp_wifi.h>

static const char *TAG = "cjf:wifi:ap";

namespace cjf
{
  std::expected<wifi_mode_soft_ap, esp_err_t> wifi_mode_soft_ap::start(
      const char *ssid,
      const char *password)
  {
    ESP_LOGW(TAG, "Starting soft AP mode");
    ESP_LOGW(TAG, "SSID: %s", ssid);

    RETURN_UNEXPECTED_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG);

    wifi_config_t wifi_config{};
    std::strncpy(reinterpret_cast<char *>(wifi_config.ap.ssid), ssid, sizeof(wifi_config.ap.ssid) - 1);
    std::strncpy(reinterpret_cast<char *>(wifi_config.ap.password), password, sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.max_connection = 4;

    RETURN_UNEXPECTED_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG);
    RETURN_UNEXPECTED_ON_ERROR(esp_wifi_start(), TAG);

    return wifi_mode_soft_ap();
  }

  void wifi_mode_soft_ap::deleter::operator()() const noexcept
  {
    ESP_LOGW(TAG, "Stopping soft AP mode");
    LOG_IF_ERROR(esp_wifi_stop(), TAG);
    LOG_IF_ERROR(esp_wifi_set_mode(WIFI_MODE_NULL), TAG);
  }

} // namespace cjf
