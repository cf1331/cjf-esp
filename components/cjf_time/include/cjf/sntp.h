#ifndef AF6840AB_95FF_458E_89CD_F81A2A41FB9F
#define AF6840AB_95FF_458E_89CD_F81A2A41FB9F

#include <array>
#include <cjf/error_handling.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_netif_sntp.h>
#include <expected>
#include <freertos/FreeRTOS.h>
#include <memory>

typedef enum
{
  CJF_SNTP_EVENT_SYNCED
} cjf_sntp_event_t;

ESP_EVENT_DECLARE_BASE(CJF_SNTP_EVENT);

namespace cjf
{
  class sntp_service
  {
  public:

    static constexpr const char* default_server = "pool.ntp.org";

    template <size_t ServersCount = 1>
    struct config
    {
      bool smooth_sync = false;
      bool server_from_dhcp = false;
      bool wait_for_sync = true;
      bool start = true;
      bool renew_servers_after_new_IP = false;
      ip_event_t ip_event_to_renew = IP_EVENT_STA_GOT_IP;
      const std::array<const char *, ServersCount> &servers;
    };

    static std::expected<std::unique_ptr<sntp_service>, esp_err_t> start();

    template <size_t ServersCount = 1>
    static std::expected<std::unique_ptr<sntp_service>, esp_err_t> start(const std::array<const char *, ServersCount> &servers);

    template <size_t ServersCount = 1>
    static std::expected<std::unique_ptr<sntp_service>, esp_err_t> start(const sntp_service::config<ServersCount> &config);

    ~sntp_service();

    esp_err_t sync();
    esp_err_t sync(TickType_t timeout);

  private:
    static std::unique_ptr<sntp_service> instance_;
    static void sync_callback_(timeval *tv);

    sntp_service();
  };

  // Template method implementations
  template <size_t ServersCount>
  std::expected<std::unique_ptr<sntp_service>, esp_err_t> sntp_service::start(const sntp_service::config<ServersCount> &config)
  {
    static_assert(ServersCount > 0, "At least one SNTP server must be specified");
    static_assert(ServersCount <= CONFIG_LWIP_SNTP_MAX_SERVERS, "Number of SNTP servers exceeds CONFIG_LWIP_SNTP_MAX_SERVERS");

    if (instance_)
    {
      return std::unexpected(ESP_ERR_INVALID_STATE);
    }

    esp_sntp_config_t sntp_config = {
        .smooth_sync = config.smooth_sync,
        .server_from_dhcp = config.server_from_dhcp,
        .wait_for_sync = config.wait_for_sync,
        .start = config.start,
        .sync_cb = sync_callback_,
        .renew_servers_after_new_IP = config.renew_servers_after_new_IP,
        .ip_event_to_renew = config.ip_event_to_renew,
        .index_of_first_server = 0,
        .num_of_servers = ServersCount,
        .servers = config.servers.front(),
    };

    RETURN_UNEXPECTED_ON_ERROR(esp_netif_sntp_init(&sntp_config), "cjf:sntp");
    return std::unique_ptr<cjf::sntp_service>(new sntp_service());
  }

  template <size_t ServersCount>
  std::expected<std::unique_ptr<sntp_service>, esp_err_t> sntp_service::start(const std::array<const char *, ServersCount> &servers)
  {
    sntp_service::config<ServersCount> config = {
        .servers = const_cast<std::array<const char *, ServersCount> &>(servers)};
    return start(config);
  }

} // namespace cjf

#endif // AF6840AB_95FF_458E_89CD_F81A2A41FB9F
