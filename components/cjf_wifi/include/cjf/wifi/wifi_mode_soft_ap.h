#ifndef EAAD9018_2F41_4B92_AFC8_5CBDEF6B73E9
#define EAAD9018_2F41_4B92_AFC8_5CBDEF6B73E9

#include <cjf/scope_guard.h>
#include <esp_err.h>
#include <expected>

namespace cjf
{

  class wifi_mode_soft_ap
  {
  public:
    static std::expected<wifi_mode_soft_ap, esp_err_t> start(const char *ssid, const char *password);

  private:
    struct deleter
    {
      void operator()() const noexcept;
    };

    cjf::scope_guard<deleter> scope_guard_;

    wifi_mode_soft_ap() = default;
  };

} // namespace cjf

#endif /* EAAD9018_2F41_4B92_AFC8_5CBDEF6B73E9 */
