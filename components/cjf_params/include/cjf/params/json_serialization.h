#ifndef E5BA0B5C_434F_4836_A516_02F544285245
#define E5BA0B5C_434F_4836_A516_02F544285245

#include "cjf/params.h"
#include <cJSON.h>
#include <esp_err.h>
#include <map>
#include <optional>

namespace cjf
{

  cJSON *to_json(const param &param);
  cJSON *to_json(const std::map<const char *, param *> &params);
  std::expected<param_value, esp_err_t> from_json(const cJSON *json);
  esp_err_t from_json(param &param, const cJSON *json);
  esp_err_t from_json(const std::map<const char*, param *> &params, const cJSON *json);

} // namespace cjf

#endif /* E5BA0B5C_434F_4836_A516_02F544285245 */
