#ifndef E47B06E7_78C7_41A8_B6D1_184565817D88
#define E47B06E7_78C7_41A8_B6D1_184565817D88

#include <cjf/event.h>
#include <cjf/nvs.h>
#include <esp_err.h>
#include <esp_netif.h>
#include <expected>
#include <memory>

namespace cjf
{

  class wifi_mode_sta
  {
  public:
    static std::expected<wifi_mode_sta, esp_err_t> start(cjf::nvs &nvs);

    esp_err_t connect() const;
    esp_err_t disconnect() const;

  private:
    struct deleter
    {
      void operator()(esp_netif_t *netif) const noexcept;
    };

    cjf::event_handler event_handler_;
    std::unique_ptr<esp_netif_t, deleter> netif_;
    std::reference_wrapper<cjf::nvs> nvs_;

    wifi_mode_sta(
        cjf::event_handler event_handler,
        std::unique_ptr<esp_netif_t, deleter> netif,
        std::reference_wrapper<cjf::nvs> nvs);
  };

} // namespace cjf

#endif /* E47B06E7_78C7_41A8_B6D1_184565817D88 */
