#include "cjf/wifi/wifi_mode_smartconfig.h"
#include "cjf/wifi/wifi_events.h"
#include <cjf/error_handling.h>
#include <cstring>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_smartconfig.h>
#include <esp_wifi.h>

static const char *TAG = "cjf:wifi:sc";
static const char *NAMESPACE = "cjf.wifi";

namespace cjf
{
  void on_got_ssid_pswd(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
  {
    auto *evt = reinterpret_cast<smartconfig_event_got_ssid_pswd_t *>(event_data);
    
    wifi_config_t wifi_config;
    std::memset(&wifi_config, 0, sizeof(wifi_config_t));
    std::memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
    std::memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
    
    ESP_LOGI(TAG, "SmartConfig received SSID: %s", wifi_config.sta.ssid);
    LOG_IF_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG);
    LOG_IF_ERROR(esp_wifi_connect(), TAG);
    esp_event_post(CJF_WIFI_EVENT, CJF_WIFI_EVENT_PROVISIONING, nullptr, 0, portMAX_DELAY);
  }

  std::expected<wifi_mode_smartconfig, esp_err_t> wifi_mode_smartconfig::start(
      std::shared_ptr<cjf::nvs> nvs)
  {
    // SmartConfig key must be 16 bytes
    static constexpr size_t key_size = 16;

    // Load SmartConfig key from NVS
    auto key = (*nvs)
        .open(NAMESPACE, NVS_READONLY)
        .and_then(
            [](const cjf::nvs_namespace &ns) -> std::expected<std::unique_ptr<char[]>, esp_err_t>
            {
              auto key = std::make_unique<char[]>(key_size + 1); // +1 for null terminator
              RETURN_UNEXPECTED_ON_ERROR(ns.get_blob("sc.key", key.get(), key_size), TAG);
              ESP_LOGI(TAG, "SmartConfig key loaded from NVS");
              key[key_size] = '\0'; // Ensure null termination
              return key;
            });

    if (!key)
    {
      ESP_LOGE(TAG, "Failed to load SmartConfig key from NVS: %s", esp_err_to_name(key.error()));
      return std::unexpected(key.error());
    }

    // Start STA mode first
    return wifi_mode_sta::start(nvs)
        .and_then([](wifi_mode_sta &&sta_mode) -> std::expected<wifi_mode_smartconfig, esp_err_t>
                  {
                    // Create SmartConfig event handler
                    auto event_handler = cjf::event_handler::create(
                        SC_EVENT,
                        SC_EVENT_GOT_SSID_PSWD,
                        on_got_ssid_pswd);
                    RETURN_UNEXPECTED_ON_ERROR(event_handler.error(), TAG);
                    
                    ESP_LOGW(TAG, "Starting SmartConfig mode");
                    return wifi_mode_smartconfig(
                        std::move(event_handler.value()),
                        std::move(sta_mode));
                  })
        .and_then(
            [&](wifi_mode_smartconfig &&smartconfig_mode) -> std::expected<wifi_mode_smartconfig, esp_err_t>
            {
              smartconfig_start_config_t config{
                  .enable_log = false,
                  .esp_touch_v2_enable_crypt = true,
                  .esp_touch_v2_key = key.value().get()};
              RETURN_UNEXPECTED_ON_ERROR(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2), TAG);
              RETURN_UNEXPECTED_ON_ERROR(esp_smartconfig_start(&config), TAG);
              return std::move(smartconfig_mode);
            });
  }

  wifi_mode_smartconfig::wifi_mode_smartconfig(
      cjf::event_handler event_handler,
      wifi_mode_sta &&sta_mode)
      : event_handler_(std::move(event_handler)),
        sta_mode_(std::move(sta_mode)) {}

  void wifi_mode_smartconfig::deleter::operator()() const noexcept
  {
    ESP_LOGW(TAG, "Stopping SmartConfig mode");
    LOG_IF_ERROR(esp_smartconfig_stop(), TAG);
  }

} // namespace cjf
