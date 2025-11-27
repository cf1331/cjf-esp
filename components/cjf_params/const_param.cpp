#include "cjf/params/const_param.h"
#include <esp_log.h>

namespace cjf
{
  static const char *CJF_CONST_PARAM = "cjf:const_param";


  const_param::const_param(const std::expected<param_value, param_error>& value)
    : value_(value)
  {
  }

  std::expected<param_value, param_error> const_param::get() const noexcept
  {
    return value_;
  }

  param_error const_param::set(const std::expected<param_value, param_error> &value)
  {
    ESP_LOGW(CJF_CONST_PARAM, "setting value is not allowed");
    return param_error::read_only;
  }

  void const_param::watch(value_changed_func callback, void* ctx)
  {
    // Value never changes so callback will never get called, no need to store
    // it.
  }

  void const_param::unwatch(value_changed_func callback)
  {
    // Callbacks aren't stored - see comment in watch().
  }
}