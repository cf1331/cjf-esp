#ifndef E12C0F9B_8C8D_4481_B6CC_295BE94B8EE6
#define E12C0F9B_8C8D_4481_B6CC_295BE94B8EE6

#include "cjf/params.h"
#include <expected>

namespace cjf
{

  class const_param : public param
  {
  public:
    const_param(const std::expected<param_value, param_error>& value);
    std::expected<param_value, param_error> get() const noexcept override;
      __attribute__((warning("the param value is read-only. const_param::set() will always return param_error::read_only")))
    param_error set(const std::expected<param_value, param_error> &value) override;
    void watch(value_changed_func callback, void *ctx = nullptr) override;
    void unwatch(value_changed_func callback) override;

  private:
    const std::expected<param_value, param_error> value_;
  };

}

#endif /* E12C0F9B_8C8D_4481_B6CC_295BE94B8EE6 */
