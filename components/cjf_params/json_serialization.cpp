#include "cjf/params/json_serialization.h"
#include <esp_err.h>

namespace cjf
{

  cJSON *to_json(const cjf::param &param)
  {
    if (!param.has_value())
    {
      return cJSON_CreateNull();
    }
    else
    {
      param_value value = param.get().value();
      return std::visit([](auto &&arg)
                        {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, bool>)
      {
        return cJSON_CreateBool(arg);
      }
      else if constexpr (std::is_same_v<T, std::string>)
      {
        return cJSON_CreateString(arg.c_str());
      }
      else
      {
        return cJSON_CreateNumber(arg);
      } },
                        value);
    }
    return nullptr;
  }

  cJSON *to_json(const std::map<const char *, cjf::param *> &params)
  {
    cJSON *json = cJSON_CreateObject();
    for (const auto &[name, param] : params)
    {
      cJSON_AddItemToObject(json, name, to_json(*param));
    }
    return json;
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
    param.set(*value);
    return ESP_OK;
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
