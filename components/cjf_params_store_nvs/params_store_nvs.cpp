#include "cjf/params_store_nvs.h"
#include <cjf/error_handling.h>
#include <esp_log.h>

namespace cjf
{

  const char *params_store_nvs::TAG = "params_store_nvs";

  params_store_nvs::params_store_nvs(passkey, nvs_namespace &&ns) noexcept
      : ns_(std::move(ns))
  {
  }

  std::expected<std::shared_ptr<params_store_nvs>, esp_err_t> params_store_nvs::open(
      std::shared_ptr<cjf::nvs> nvs,
      const char *namespace_name) noexcept
  {
    auto ns = nvs->open(namespace_name, NVS_READWRITE);
    RETURN_ON_UNEXPECTED(ns, TAG, "Failed to open NVS namespace: %s, error: %s", namespace_name, esp_err_to_name(ns.error()));

    // Use make_shared with passkey - this properly initializes enable_shared_from_this
    auto store = std::make_shared<params_store_nvs>(passkey{}, std::move(*ns));
    ESP_LOGI(TAG, "Opened NVS namespace: %s", namespace_name);
    return store;
  }

  esp_err_t params_store_nvs::erase(const char *key) noexcept
  {
    esp_err_t err = ns_.erase(key);
    if (err == ESP_OK)
    {
      ns_.commit();
      ESP_LOGI(TAG, "%s (erased)", key);
    }
    else if (err != ESP_ERR_NVS_NOT_FOUND)
    {
      ESP_LOGW(TAG, "Failed to erase %s: %s", key, esp_err_to_name(err));
    }
    return err;
  }

} // namespace cjf