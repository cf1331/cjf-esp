#ifndef E352C266_7032_4AD5_9C7A_B18D9968AF29
#define E352C266_7032_4AD5_9C7A_B18D9968AF29

#include "cjf/wifi/wifi_events.h"
#include "cjf/wifi/wifi_mode_sta.h"
#include "cjf/wifi/wifi_mode_soft_ap.h"
#include "cjf/wifi/wifi_mode_smartconfig.h"
#include <cjf/nvs.h>
#include <esp_err.h>
#include <expected>
#include <memory>
#include <optional>
#include <variant>

namespace cjf
{
  class wifi
  {
  public:
    using mode_type = std::variant<
        wifi_mode_smartconfig,
        wifi_mode_soft_ap,
        wifi_mode_sta>;

    static std::expected<wifi, esp_err_t> init(
        std::shared_ptr<cjf::nvs> nvs);

    // Move-only semantics to prevent copying
    wifi(const wifi &) = delete;
    wifi &operator=(const wifi &) = delete;
    wifi(wifi &&) noexcept = default;
    wifi &operator=(wifi &&) noexcept = default;
    ~wifi() = default;

    esp_err_t connect();
    esp_err_t disconnect();
    esp_err_t provision();
    esp_err_t soft_ap(const char *ssid, const char *password);

    std::optional<mode_type> &mode() noexcept;

  private:
    struct deleter
    {
      void operator()() const noexcept;
    };

    cjf::scope_guard<deleter> cleanup_;
    std::optional<mode_type> mode_;
    std::shared_ptr<cjf::nvs> nvs_;

    explicit wifi(cjf::scope_guard<deleter> cleanup, std::shared_ptr<cjf::nvs> nvs);

    esp_err_t change_mode_(mode_type &&new_mode);
    esp_err_t change_mode_(std::expected<mode_type, esp_err_t> new_mode);
  };

} // namespace cjf

#endif // E352C266_7032_4AD5_9C7A_B18D9968AF29
