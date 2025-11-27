#include "cjf/wifi.h"
#include <cjf/error_handling.h>
#include <cstring>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_smartconfig.h>
#include <esp_wifi.h>
#include <expected>
#include <memory>
#include <tuple>

static const char *CJF_WIFI = "cjf:wifi";
static const char *CJF_WIFI_NAMESPACE = "cjf.wifi";

namespace cjf
{
  // Singleton instance
  std::unique_ptr<wifi> wifi::instance_;

  template <class T>
  T *get_mode_if(wifi *wifi)
  {
    return wifi->mode() ? std::get_if<T>(&wifi->mode().value()) : nullptr;
  }

  static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
  {
    auto wifi = reinterpret_cast<cjf::wifi *>(arg);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
      auto *evt = reinterpret_cast<wifi_event_sta_disconnected_t *>(event_data);
      auto *sta_mode = get_mode_if<wifi_mode_sta>(wifi);
      ESP_LOGI(CJF_WIFI, "Disconnected from SSID: %s, reason: %i", evt->ssid, evt->reason);
      switch (evt->reason)
      {
      case WIFI_REASON_ASSOC_LEAVE:
        // This occurs when esp_wifi_disconnect() is called. Assume the
        // disconnection is intentional and don't try to reconnect.
        break;

      case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
      case WIFI_REASON_HANDSHAKE_TIMEOUT:
        ESP_LOGE(CJF_WIFI, "Wi-Fi password may be incorrect");
        break;

      default:
        sta_mode->reconnect();
      }
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
      auto *evt = reinterpret_cast<smartconfig_event_got_ssid_pswd_t *>(event_data);
      auto *wifi = reinterpret_cast<cjf::wifi *>(arg);
      wifi_mode_smartconfig::got_ssid_pswd(evt, wifi);
    }
  }

  std::expected<wifi_mode_sta, esp_err_t> wifi_mode_sta::connect()
  {
    return start()
        .and_then([](wifi_mode_sta &&sta_mode) -> std::expected<wifi_mode_sta, esp_err_t>
                  {
        RETURN_UNEXPECTED_ON_ERROR(sta_mode.reconnect(), CJF_WIFI);
        return std::move(sta_mode); });
  }

  std::expected<wifi_mode_sta, esp_err_t> wifi_mode_sta::start()
  {
    ESP_LOGW(CJF_WIFI, "Starting sta mode");
    netif_type netif(esp_netif_create_default_wifi_sta());
    if (!netif)
    {
      return std::unexpected(ESP_ERR_NO_MEM);
    }
    RETURN_UNEXPECTED_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), CJF_WIFI);
    RETURN_UNEXPECTED_ON_ERROR(esp_wifi_start(), CJF_WIFI);
    return wifi_mode_sta(std::move(netif));
  }

  esp_err_t wifi_mode_sta::disconnect() const
  {
    ESP_LOGI(CJF_WIFI, "Disconnecting from current network");
    RETURN_ON_ERROR(esp_wifi_disconnect(), CJF_WIFI);
    return ESP_OK;
  }

  esp_err_t wifi_mode_sta::reconnect() const
  {
    esp_event_post(CJF_WIFI_EVENT, CJF_WIFI_EVENT_CONNECTING, nullptr, 0, portMAX_DELAY);
    wifi_config_t wifi_config;
    RETURN_ON_ERROR(esp_wifi_get_config(WIFI_IF_STA, &wifi_config), CJF_WIFI);
    ESP_LOGI(CJF_WIFI, "Connecting to SSID: %s", wifi_config.sta.ssid);
    RETURN_ON_ERROR(esp_wifi_connect(), CJF_WIFI);
    return ESP_OK;
  }

  void wifi_mode_sta::deleter::operator()(esp_netif_t *netif) const noexcept
  {
    ESP_LOGW(CJF_WIFI, "Stopping sta mode");
    LOG_IF_ERROR(esp_wifi_disconnect(), CJF_WIFI);
    LOG_IF_ERROR(esp_wifi_stop(), CJF_WIFI);
    if (netif)
    {
      esp_netif_destroy_default_wifi(netif);
    }
  }

  wifi_mode_sta::wifi_mode_sta(netif_type &&netif)
      : netif_(std::move(netif)) {}

  std::expected<wifi_mode_soft_ap, esp_err_t> start(const char *ssid, const char *password)
  {
    return std::unexpected(ESP_ERR_NOT_SUPPORTED);
  }

  void wifi_mode_soft_ap::deleter::operator()() const noexcept
  {
    ESP_LOGW(CJF_WIFI, "Stopping soft AP mode");
  }

  std::expected<wifi_mode_smartconfig, esp_err_t> wifi_mode_smartconfig::start(
      std::expected<cjf::nvs, esp_err_t> &nvs)
  {
    // SmartConfig key must be 16 bytes
    static constexpr size_t key_size = 16;

    auto key =
        nvs
            .and_then([](cjf::nvs &nvs)
                      { return nvs.open(CJF_WIFI_NAMESPACE, NVS_READONLY); })
            .and_then(
                [&](const cjf::nvs_namespace &ns) -> std::expected<std::unique_ptr<char[]>, esp_err_t>
                {
                  auto key = std::make_unique<char[]>(key_size + 1); // +1 for null terminator
                  RETURN_UNEXPECTED_ON_ERROR(ns.get_blob("sc.key", key.get(), key_size), CJF_WIFI);
                  ESP_LOGI(CJF_WIFI, "SmartConfig key loaded from NVS:");
                  ESP_LOG_BUFFER_HEXDUMP(CJF_WIFI, key.get(), key_size, ESP_LOG_INFO);
                  key[key_size] = '\0'; // Ensure null termination
                  return key;
                });

    if (!key)
    {
      ESP_LOGE(CJF_WIFI, "Failed to load SmartConfig key from NVS: %s", esp_err_to_name(key.error()));
      return std::unexpected(key.error());
    }

    return wifi_mode_sta::start()
        .and_then([](wifi_mode_sta &&sta_mode) -> std::expected<wifi_mode_smartconfig, esp_err_t>
                  {
                    ESP_LOGW(CJF_WIFI, "Starting SmartConfig mode");
                    return wifi_mode_smartconfig(std::move(sta_mode)); })
        .and_then(
            [&](wifi_mode_smartconfig &&smartconfig_mode) -> std::expected<wifi_mode_smartconfig, esp_err_t>
            {
              smartconfig_start_config_t config{
                  .enable_log = false,
                  .esp_touch_v2_enable_crypt = true,
                  .esp_touch_v2_key = key.value().get()};
              RETURN_UNEXPECTED_ON_ERROR(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2), CJF_WIFI);
              RETURN_UNEXPECTED_ON_ERROR(esp_smartconfig_start(&config), CJF_WIFI);
              esp_event_post(CJF_WIFI_EVENT, CJF_WIFI_EVENT_PROVISIONING, nullptr, 0, portMAX_DELAY);
              return std::move(smartconfig_mode);
            });
  }

  void wifi_mode_smartconfig::got_ssid_pswd(
      smartconfig_event_got_ssid_pswd_t *evt,
      cjf::wifi *wifi)
  {
    auto *sc_mode = get_mode_if<wifi_mode_smartconfig>(wifi);
    if (!sc_mode)
    {
      ESP_LOGE(CJF_WIFI, "Not in SmartConfig mode");
      return;
    }
    ESP_LOGI(CJF_WIFI, "SmartConfig received SSID: %s, Password: %s", evt->ssid, evt->password);
    RETURN_VOID_ON_ERROR(sc_mode->sta_mode_.disconnect(), CJF_WIFI);
    // Save the SSID and password to NVS
    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
    RETURN_VOID_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), CJF_WIFI);
    // Start connecting to the newly saved network
    RETURN_VOID_ON_ERROR(sc_mode->sta_mode_.reconnect(), CJF_WIFI);
    wifi::mode_type sta_mode(std::move(sc_mode->sta_mode_));
    wifi->change_mode_(std::move(sta_mode));
  }

  wifi_mode_smartconfig::wifi_mode_smartconfig(wifi_mode_sta &&sta_mode)
      : sta_mode_(std::move(sta_mode)) {}

  void wifi_mode_smartconfig::deleter::operator()() const noexcept
  {
    ESP_LOGW(CJF_WIFI, "Stopping SmartConfig mode");
    LOG_IF_ERROR(esp_smartconfig_stop(), CJF_WIFI);
  }

  std::expected<singleton<wifi>, esp_err_t> wifi::init(
      std::expected<cjf::nvs, esp_err_t> &nvs)
  {
    if (instance_)
    {
      ESP_LOGW(CJF_WIFI, "Wifi already initialized");
      return std::unexpected(ESP_ERR_INVALID_STATE);
    }
    ESP_LOGI(CJF_WIFI, "Initializing wifi");
    // The default event loop is required by esp_wifi. If it has already been created,
    // esp_event_loop_create_default() will return ESP_ERR_INVALID_STATE.
    esp_err_t res = esp_event_loop_create_default();
    if (res != ESP_OK && res != ESP_ERR_INVALID_STATE)
    {
      RETURN_UNEXPECTED_ON_ERROR(res, CJF_WIFI);
    }
    RETURN_UNEXPECTED_ON_ERROR(esp_netif_init(), CJF_WIFI);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    RETURN_UNEXPECTED_ON_ERROR(esp_wifi_init(&cfg), CJF_WIFI);
    // Create the singleton instance
    instance_ = std::unique_ptr<wifi>(new wifi(nvs));
    // Event handlers are registered on the instance after it's created so that
    // the instance can be accessed from the event handler.
    res = instance_->register_event_handlers_();
    if (res != ESP_OK)
    {
      instance_.reset();
      return std::unexpected(res);
    }
    ESP_LOGI(CJF_WIFI, "Initialized wifi");
    return singleton(instance_);
  }

  wifi::~wifi()
  {
    ESP_LOGW(CJF_WIFI, "Destroying wifi instance");
    // Clean up ESP-IDF wifi resources
    LOG_IF_ERROR(unregister_event_handlers_(), CJF_WIFI);
    LOG_IF_ERROR(esp_wifi_deinit(), CJF_WIFI);
    LOG_IF_ERROR(esp_netif_deinit(), CJF_WIFI);
  }

  esp_err_t wifi::connect()
  {
    disconnect();
    return change_mode_(wifi_mode_sta::connect());
  }

  esp_err_t wifi::disconnect()
  {
    if (mode_)
    {
      ESP_LOGI(CJF_WIFI, "Disconnecting");
      mode_.reset();
    }
    return ESP_OK;
  }

  esp_err_t wifi::provision()
  {
    disconnect();
    return change_mode_(wifi_mode_smartconfig::start(nvs_));
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

  wifi::wifi(std::expected<cjf::nvs, esp_err_t> &nvs)
      : mode_(std::nullopt), nvs_(nvs), wifi_event_handler_(nullptr) {}

  esp_err_t wifi::change_mode_(mode_type &&new_mode)
  {
    mode_ = std::move(new_mode);
    return ESP_OK;
  }

  esp_err_t wifi::change_mode_(std::expected<mode_type, esp_err_t> new_mode)
  {
    return (new_mode) ? change_mode_(std::move(new_mode.value()))
                      : new_mode.error();
  }

  esp_err_t wifi::register_event_handlers_()
  {
    RETURN_ON_ERROR(esp_event_handler_instance_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, this, &sc_event_handler_), CJF_WIFI);
    RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, this, &wifi_event_handler_), CJF_WIFI);
    return ESP_OK;
  }

  esp_err_t wifi::unregister_event_handlers_()
  {
    if (sc_event_handler_)
    {
      RETURN_ON_ERROR(esp_event_handler_instance_unregister(SC_EVENT, ESP_EVENT_ANY_ID, sc_event_handler_), CJF_WIFI);
      sc_event_handler_ = nullptr;
    }
    if (wifi_event_handler_)
    {
      RETURN_ON_ERROR(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler_), CJF_WIFI);
      wifi_event_handler_ = nullptr;
    }
    return ESP_OK;
  }

} // namespace cjf
