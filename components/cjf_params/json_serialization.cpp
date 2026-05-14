#include "cjf/params/json_serialization.h"
#include <cstring>
#include <esp_err.h>

namespace cjf
{

  std::expected<std::string_view, esp_err_t> to_json(char *first, char *last, std::string_view str)
  {
    if (std::distance(first, last) < 2) return std::unexpected(ESP_ERR_NO_MEM);
    char *p = first;
    *p++ = '"';

    const char *run_start = str.data();
    const char *const str_end = str.data() + str.size();

    for (const char *src = str.data(); src != str_end; ++src)
    {
      const unsigned char c = static_cast<unsigned char>(*src);
      std::string_view seq;
      switch (c)
      {
      case '"':  seq = "\\\""; break;
      case '\\': seq = "\\\\"; break;
      case '\b': seq = "\\b";  break;
      case '\f': seq = "\\f";  break;
      case '\n': seq = "\\n";  break;
      case '\r': seq = "\\r";  break;
      case '\t': seq = "\\t";  break;
      default:
        // JSON spec (RFC 8259) requires control characters in the range U+0000 to U+001F to be escaped as \uXXXX
        if (c < 0x20)
        {
          char ctrl[6] = {'\\', 'u', '0', '0',
            "0123456789abcdef"[(c >> 4) & 0xf],
            "0123456789abcdef"[c & 0xf]};
          seq = {ctrl, sizeof(ctrl)};
        }
        break;
      }

      // Plain character - extend the run
      if (seq.empty()) continue;

      // Flush the plain run up to this character, then write the escape sequence
      const size_t run_len = static_cast<size_t>(src - run_start);
      if (std::distance(p, last) < static_cast<ptrdiff_t>(run_len + seq.size() + 1))
        return std::unexpected(ESP_ERR_NO_MEM);
      std::memcpy(p, run_start, run_len);
      p += run_len;
      std::memcpy(p, seq.data(), seq.size());
      p += seq.size();
      run_start = src + 1;
    }

    // Flush the final plain run
    const size_t run_len = static_cast<size_t>(str_end - run_start);
    if (std::distance(p, last) < static_cast<ptrdiff_t>(run_len + 1))
      return std::unexpected(ESP_ERR_NO_MEM);
    std::memcpy(p, run_start, run_len);
    p += run_len;

    *p++ = '"';
    return std::string_view{first, static_cast<size_t>(p - first)};
  }


  std::expected<std::string_view, esp_err_t> to_json(char *first, char *last, const param_value &value)
  {
    if (std::holds_alternative<std::monostate>(value))
    {
      static const auto null_value = std::string_view{"null"};
      if (std::distance(first, last) < static_cast<ptrdiff_t>(null_value.size()))
        return std::unexpected(ESP_ERR_NO_MEM);
      std::copy_n(null_value.cbegin(), null_value.size(), first);
      return std::string_view{first, null_value.size()};
    }
    if (std::holds_alternative<std::string_view>(value))
      return to_json(first, last, std::get<std::string_view>(value));
    auto result = to_chars(first, last, value);
    if (!result) return std::unexpected(ESP_ERR_NO_MEM);
    return result.value();
  }

  std::expected<std::string_view, esp_err_t> to_json(char *first, char *last, const cjf::param &param)
  {
    return to_json(first, last, param.get());
  }

  std::expected<std::string_view, esp_err_t> to_json(char *first, char *last, const std::map<const char *, cjf::param *> &params)
  {
    char *p = first;
    if (std::distance(first, last) < 2) return std::unexpected(ESP_ERR_NO_MEM);
    *p++ = '{';
    bool first_entry = true;
    for (const auto &[name, param] : params)
    {
      if (!first_entry)
      {
        if (p >= last) return std::unexpected(ESP_ERR_NO_MEM);
        *p++ = ',';
      }
      first_entry = false;
      // Write quoted key
      auto key_sv = to_json(p, last, std::string_view{name});
      if (!key_sv) return std::unexpected(key_sv.error());
      p += key_sv->size();
      if (p >= last) return std::unexpected(ESP_ERR_NO_MEM);
      *p++ = ':';
      // Write value
      auto value_sv = to_json(p, last, *param);
      if (!value_sv) return std::unexpected(value_sv.error());
      p += value_sv->size();
    }
    if (p >= last) return std::unexpected(ESP_ERR_NO_MEM);
    *p++ = '}';
    return std::string_view{first, static_cast<size_t>(p - first)};
  }

  std::expected<param_value, esp_err_t> from_json(const cJSON *json)
  {
    std::optional<param_value> value;
    if (cJSON_IsBool(json))
    {
      return cJSON_IsTrue(json) ? true : false;
    }
    else if (cJSON_IsNumber(json))
    {
      double n = cJSON_GetNumberValue(json);
      int64_t n_int = static_cast<int64_t>(n);
      return (n == n_int) ? n_int : n;
    }
    else if (cJSON_IsNull(json))
    {
      return param_null;
    }
    else if (cJSON_IsString(json))
    {
      return cJSON_GetStringValue(json);
    }
    else
    {
      return std::unexpected(ESP_ERR_INVALID_ARG);
    }
  }

  esp_err_t from_json(param &param, const cJSON *json)
  {
    auto value = from_json(json);
    if (!value)
    {
      return value.error();
    }
    param_error err = param.set(*value);
    return (err == param_error::ok) ? ESP_OK : ESP_ERR_INVALID_ARG;
  }

  esp_err_t from_json(const std::map<const char *, param *> &params, const cJSON *json)
  {
    for (const auto &[name, param] : params)
    {
      cJSON *item = cJSON_GetObjectItemCaseSensitive(json, name);
      if (item)
      {
        esp_err_t err = from_json(*param, item);
        if (err != ESP_OK)
        {
          return err;
        }
      }
    }
    return ESP_OK;
  }

} // namespace cjf
