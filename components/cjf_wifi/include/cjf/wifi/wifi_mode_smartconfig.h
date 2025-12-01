#ifndef FC144B1E_8930_4938_BB2E_6D941424F810
#define FC144B1E_8930_4938_BB2E_6D941424F810

#include "cjf/wifi/wifi_mode_sta.h"
#include <cjf/event.h>
#include <cjf/nvs.h>
#include <cjf/scope_guard.h>
#include <esp_err.h>
#include <esp_smartconfig.h>
#include <expected>

namespace cjf
{
  // Forward declaration of wifi class
  class wifi;

  class wifi_mode_smartconfig
  {
  public:
    static std::expected<wifi_mode_smartconfig, esp_err_t> start(
        std::shared_ptr<cjf::nvs> nvs);

  private:
    struct deleter
    {
      void operator()() const noexcept;
    };

    cjf::event_handler event_handler_;
    cjf::scope_guard<deleter> scope_guard_;
    wifi_mode_sta sta_mode_;

    wifi_mode_smartconfig(
        cjf::event_handler event_handler,
        wifi_mode_sta &&sta_mode);
  };

} // namespace cjf

#endif /* FC144B1E_8930_4938_BB2E_6D941424F810 */
