#include "cjf/wifi.h"
#include "cjf/wifi/wifi_mode_smartconfig.h"
#include "cjf/wifi/wifi_mode_soft_ap.h"
#include "cjf/wifi/wifi_mode_sta.h"
#include <cjf/error_handling.h>
#include <esp_log.h>
#include <esp_wifi.h>

static const char *TAG = "cjf:wifi";

namespace cjf
{
  std::expected<wifi, esp_err_t> wifi::init(std::expected<cjf::nvs, esp_err_t> &nvs)
  {
    ESP_LOGI(TAG, "Initializing wifi");

    // The default event loop is required by esp_wifi. If it has already been created,
    // esp_event_loop_create_default() will return ESP_ERR_INVALID_STATE.
    esp_err_t res = esp_event_loop_create_default();
    if (res != ESP_OK && res != ESP_ERR_INVALID_STATE)
    {
      RETURN_UNEXPECTED_ON_ERROR(res, TAG);
    }
    RETURN_ON_UNEXPECTED(nvs, TAG);

    cjf::scope_guard<deleter> cleanup;
    RETURN_UNEXPECTED_ON_ERROR(esp_netif_init(), TAG);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    RETURN_UNEXPECTED_ON_ERROR(esp_wifi_init(&cfg), TAG);

    ESP_LOGI(TAG, "Initialized wifi");
    return wifi(std::move(cleanup), std::ref(*nvs));
  }

  wifi::wifi(cjf::scope_guard<deleter> cleanup, std::reference_wrapper<cjf::nvs> nvs)
      : cleanup_(std::move(cleanup)), mode_(std::nullopt), nvs_(nvs) {}

  void wifi::deleter::operator()() const noexcept
  {
    ESP_LOGW(TAG, "Destroying wifi instance");
    // Clean up ESP-IDF wifi resources
    LOG_IF_ERROR(esp_wifi_deinit(), TAG);
    LOG_IF_ERROR(esp_netif_deinit(), TAG);
  }

  esp_err_t wifi::connect()
  {
    disconnect();
    auto sta = wifi_mode_sta::start(nvs_.get());
    RETURN_ERROR_ON_UNEXPECTED(sta, TAG);
    sta->connect();
    return change_mode_(std::move(sta));
  }

  esp_err_t wifi::disconnect()
  {
    if (mode_)
    {
      ESP_LOGI(TAG, "Disconnecting");
      mode_.reset();
    }
    return ESP_OK;
  }

  esp_err_t wifi::provision()
  {
    disconnect();
    return change_mode_(wifi_mode_smartconfig::start(nvs_.get()));
  }

  esp_err_t wifi::soft_ap(const char *ssid, const char *password)
  {
    disconnect();
    return change_mode_(wifi_mode_soft_ap::start(ssid, password));
  }

  std::optional<wifi::mode_type> &wifi::mode() noexcept
  {
    return mode_;
  }

  esp_err_t wifi::change_mode_(mode_type &&new_mode)
  {
    mode_ = std::move(new_mode);
    return ESP_OK;
  }

  esp_err_t wifi::change_mode_(std::expected<mode_type, esp_err_t> new_mode)
  {
    if (new_mode)
    {
      return change_mode_(std::move(new_mode.value()));
    }
    return new_mode.error();
  }

} // namespace cjf
