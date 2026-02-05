#include "cjf/time.h"
#include <cstring>
#include <esp_log.h>

namespace cjf
{
  static const char *CJF_TIME = "cjf:time";
  static const char *CJF_TIME_NAMESPACE = "cjf.time";

  esp_err_t set_timezone(const char *tz)
  {
    if (setenv("TZ", tz, 1) != 0)
    {
      ESP_LOGE(CJF_TIME, "Failed to set timezone environment variable");
      return ESP_ERR_INVALID_STATE;
    }
    tzset(); // Update the timezone settings
    return ESP_OK;
  }

  esp_err_t load_timezone(std::expected<cjf::nvs, esp_err_t> &nvs, const char *default_tz)
  {
    RETURN_ERROR_ON_FALSE(nvs, ESP_ERR_INVALID_ARG, CJF_TIME, "nvs not initialized");
    char tz[64] = {0};
    auto cjf_ns = nvs->open(CJF_TIME_NAMESPACE, NVS_READWRITE);
    if (cjf_ns)
    {
      cjf_ns->get_string("tz", tz, sizeof(tz));
      if (!tz[0])
      {
        cjf_ns->set_string("tz", default_tz);
        cjf_ns->commit();
      }
    }
    return set_timezone(tz[0] ? tz : default_tz);
  }

  char *to_iso8061(const timeval &tv, char *dst, size_t dst_size)
  {
    struct tm timeinfo;
    localtime_r(&tv.tv_sec, &timeinfo);
    // Format the main timestamp part
    size_t len = strftime(dst, dst_size, "%Y-%m-%dT%H:%M:%S", &timeinfo);
    // Append milliseconds part
    len += snprintf(dst + len, dst_size - len, ".%03ld", tv.tv_usec / 1000);
    // Append timezone offset part
    strftime(dst + len, dst_size - len, "%z", &timeinfo);
    return dst;
  }

  std::string to_iso8061(const timeval &tv)
  {
    // 29 chars max for ISO 8601 + null terminator
    std::string str(32, '\0');
    to_iso8061(tv, str.data(), str.size());
    str.resize(std::strlen(str.c_str())); // Trim to actual string length
    return str;
  }

} // namespace cjf