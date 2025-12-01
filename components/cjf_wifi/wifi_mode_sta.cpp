#include "cjf/wifi/wifi_mode_sta.h"
#include "cjf/wifi/wifi_events.h"
#include <cjf/error_handling.h>
#include <esp_wifi.h>
#include <magic_enum/magic_enum.hpp>

namespace cjf
{
  static const char *TAG = "cjf:wifi:sta";

  esp_err_t connect_()
  {
    esp_event_post(CJF_WIFI_EVENT, CJF_WIFI_EVENT_CONNECTING, nullptr, 0, portMAX_DELAY);
    wifi_config_t wifi_config;
    RETURN_ON_ERROR(esp_wifi_get_config(WIFI_IF_STA, &wifi_config), TAG);
    ESP_LOGI(TAG, "Connecting to SSID: %s", wifi_config.sta.ssid);
    RETURN_ON_ERROR(esp_wifi_connect(), TAG);
    return ESP_OK;
  }

  esp_err_t disconnect_()
  {
    ESP_LOGI(TAG, "Disconnecting from current network");
    RETURN_ON_ERROR(esp_wifi_disconnect(), TAG);
    return ESP_OK;
  }

  void on_disconnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
  {
    auto *evt = reinterpret_cast<wifi_event_sta_disconnected_t *>(event_data);
    switch (evt->reason)
    {
    case WIFI_REASON_ASSOC_LEAVE:
      // This occurs when esp_wifi_disconnect() is called. Assume the
      // disconnection is intentional and don't try to reconnect.
      break;

    case WIFI_REASON_NO_AP_FOUND:
      ESP_LOGW(TAG, "Wi-Fi access point not found");
      connect_(); // Try again
      break;

    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
      ESP_LOGE(TAG, "Wi-Fi password may be incorrect");
      break;

    default:
      // In all other cases try to reconnect
      connect_();
    }
  }

  std::expected<wifi_mode_sta, esp_err_t> wifi_mode_sta::start(std::shared_ptr<cjf::nvs> nvs)
  {
    ESP_LOGW(TAG, "Starting sta mode");
    auto event_handler = cjf::event_handler::create(
        WIFI_EVENT,
        WIFI_EVENT_STA_DISCONNECTED,
        on_disconnected);
    RETURN_ON_UNEXPECTED(event_handler, TAG);

    std::unique_ptr<esp_netif_t, deleter> netif(esp_netif_create_default_wifi_sta());
    RETURN_UNEXPECTED_ON_FALSE(netif, ESP_ERR_NO_MEM, TAG);
    RETURN_UNEXPECTED_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG);
    RETURN_UNEXPECTED_ON_ERROR(esp_wifi_start(), TAG);
    return wifi_mode_sta(std::move(*event_handler), std::move(netif), nvs);
  }

  void wifi_mode_sta::deleter::operator()(esp_netif_t *netif) const noexcept
  {
    ESP_LOGW(TAG, "Stopping sta mode");
    LOG_IF_ERROR(esp_wifi_disconnect(), TAG);
    LOG_IF_ERROR(esp_wifi_stop(), TAG);
    LOG_IF_ERROR(esp_wifi_set_mode(WIFI_MODE_NULL), TAG);
    if (netif)
    {
      esp_netif_destroy_default_wifi(netif);
    }
  }

  wifi_mode_sta::wifi_mode_sta(
      cjf::event_handler event_handler,
      std::unique_ptr<esp_netif_t, deleter> netif,
      std::shared_ptr<cjf::nvs> nvs)
      : event_handler_(std::move(event_handler)),
        netif_(std::move(netif)),
        nvs_(std::move(nvs)) {}

  esp_err_t wifi_mode_sta::connect() const
  {
    return connect_();
  }

  esp_err_t wifi_mode_sta::disconnect() const
  {
    return disconnect_();
  }

} // namespace cjf
