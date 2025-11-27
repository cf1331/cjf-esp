#ifndef E352C266_7032_4AD5_9C7A_B18D9968AF29
#define E352C266_7032_4AD5_9C7A_B18D9968AF29

#include <cjf/nvs.h>
#include <cjf/scope_guard.h>
#include <cjf/singleton.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_smartconfig.h>
#include <expected>
#include <memory>
#include <optional>
#include <variant>


typedef enum
{
  CJF_WIFI_EVENT_CONNECTING,
  CJF_WIFI_EVENT_PROVISIONING,
  CJF_WIFI_EVENT_AUTH_FAILED,
} cjf_wifi_event_t;

ESP_EVENT_DEFINE_BASE(CJF_WIFI_EVENT);

namespace cjf
{
  // Forward declaration of wifi class to use in smartconfig event handler
  class wifi;

  class wifi_mode_soft_ap
  {
  public:
    static std::expected<wifi_mode_soft_ap, esp_err_t> start(const char *ssid, const char *password);

  private:
    struct deleter
    {
      void operator()() const noexcept;
    };

    cjf::scope_guard<wifi_mode_soft_ap::deleter> scope_guard_;

    wifi_mode_soft_ap() = default;
  };

  class wifi_mode_sta
  {
  public:
    static std::expected<wifi_mode_sta, esp_err_t> connect();
    static std::expected<wifi_mode_sta, esp_err_t> start();

    esp_err_t disconnect() const;
    esp_err_t reconnect() const;

  private:
    struct deleter
    {
      void operator()(esp_netif_t *netif) const noexcept;
    };
    using netif_type = std::unique_ptr<esp_netif_t, wifi_mode_sta::deleter>;
    netif_type netif_;
    wifi_mode_sta(netif_type &&netif);
  };

  class wifi_mode_smartconfig
  {
  public:
    static std::expected<wifi_mode_smartconfig, esp_err_t> start(
        std::expected<cjf::nvs, esp_err_t> &nvs);

    static void got_ssid_pswd(
        smartconfig_event_got_ssid_pswd_t *event_data,
        cjf::wifi *wifi_instance);

  private:
    struct deleter
    {
      void operator()() const noexcept;
    };

    cjf::scope_guard<wifi_mode_smartconfig::deleter> scope_guard_;
    wifi_mode_sta sta_mode_;

    wifi_mode_smartconfig(wifi_mode_sta &&sta_mode);
  };

  class wifi
  {
  public:
    using mode_type = std::variant<
        wifi_mode_smartconfig,
        wifi_mode_soft_ap,
        wifi_mode_sta>;

    static std::expected<singleton<wifi>, esp_err_t> init(
        std::expected<cjf::nvs, esp_err_t> &nvs);

    // Only a single instance of wifi can exist. It must be created by wifi::init().
    // Delete copy and move constructors and assignment operators to enforce this.
    wifi(const wifi &) = delete;
    wifi &operator=(const wifi &) = delete;
    wifi(wifi &&) = delete;
    wifi &operator=(wifi &&) = delete;

    esp_err_t connect();
    esp_err_t disconnect();
    esp_err_t provision();
    esp_err_t soft_ap(const char *ssid, const char *password);

    std::optional<mode_type> &mode() noexcept;

  private:
    static std::unique_ptr<wifi> instance_;

    std::optional<mode_type> mode_;
    std::expected<cjf::nvs, esp_err_t> &nvs_;
    esp_event_handler_instance_t sc_event_handler_;
    esp_event_handler_instance_t wifi_event_handler_;

    wifi(std::expected<cjf::nvs, esp_err_t> &nvs);
    ~wifi(); // Destructor to handle cleanup

    friend class std::default_delete<wifi>;
    friend class wifi_mode_smartconfig;

    esp_err_t change_mode_(mode_type &&new_mode);
    esp_err_t change_mode_(std::expected<mode_type, esp_err_t> new_mode);
    esp_err_t register_event_handlers_();
    esp_err_t unregister_event_handlers_();
  };

} // namespace cjf

#endif // E352C266_7032_4AD5_9C7A_B18D9968AF29
