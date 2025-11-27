#ifndef BA7523FD_3010_4D83_AEB0_11C7791E6112
#define BA7523FD_3010_4D83_AEB0_11C7791E6112

#include "cjf/params.h"
#include "cjf/watchable.h"
#include <expected>

namespace cjf
{

  template <typename T>
  class mutable_param : public param
  {
  public:
    mutable_param(const std::expected<param_value, param_error>& value = param_null);
    std::expected<param_value, param_error> get() const noexcept override;
    param_error set(const std::expected<param_value, param_error> &value) override;
    void watch(value_changed_func callback, void *ctx = nullptr) override;
    void unwatch(value_changed_func callback);

  private:
    std::expected<param_value, param_error> value_;
    watchable<param> watchable_;
  };

  template <typename T>
  inline mutable_param<T>::mutable_param(const std::expected<param_value, param_error>& value)
      : value_(value)
  {
  }

  template <typename T>
  inline std::expected<param_value, param_error> mutable_param<T>::get() const noexcept
  {
    return value_;
  }

  template <typename T>
  inline param_error mutable_param<T>::set(const std::expected<param_value, param_error> &value)
  {
    // Ensure that value is castable to T
    auto new_value = param_cast<T>(value);
    if (!new_value)
    {
      return new_value.error();
    }
    value_ = new_value;
    watchable_.notify(*this);
    return param_error::ok;
  }

  template <typename T>
  inline void mutable_param<T>::watch(value_changed_func callback, void *ctx)
  {
    watchable_.watch(callback, ctx);
  }

  template <typename T>
  inline void mutable_param<T>::unwatch(value_changed_func callback)
  {
    watchable_.unwatch(callback);
  }

} // namespace cjf

#endif /* BA7523FD_3010_4D83_AEB0_11C7791E6112 */
