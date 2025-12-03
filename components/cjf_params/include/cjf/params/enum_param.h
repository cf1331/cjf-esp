#ifndef C935C86A_5828_4BCC_900C_D24090203B65
#define C935C86A_5828_4BCC_900C_D24090203B65

#include "cjf/params.h"
#include "cjf/watchable.h"
#include <expected>
#include <magic_enum/magic_enum.hpp>

namespace cjf
{

  template <typename Enum>
  class enum_param : public param
  {
  public:
    enum_param(const Enum value) noexcept;
    enum_param() noexcept;
    enum_param(const param_value &value) noexcept;
    param_value get() const noexcept override;
    std::optional<Enum> get_enum() const noexcept;
    bool operator==(const Enum value) const noexcept;
    param_error set(const param_value &value) noexcept override;
    void set_enum(const Enum value) noexcept;
    void watch(value_changed_func callback, void *ctx = nullptr) override;
    void unwatch(value_changed_func callback) override;

  private:
    std::optional<Enum> _value_enum;
    watchable<param> _watchable;
  };

  template <typename Enum>
  inline enum_param<Enum>::enum_param(const Enum value) noexcept
      : _value_enum(value)
  {
  }

  template <typename Enum>
  inline enum_param<Enum>::enum_param() noexcept
      : _value_enum(std::nullopt)
  {
  }

  template <typename Enum>
  inline enum_param<Enum>::enum_param(const param_value &value) noexcept
      : _value_enum(std::visit([](auto &&v) -> std::optional<Enum>
                               {
                                  if constexpr (std::is_same_v<std::decay_t<decltype(v)>, null_type>)
                                  {
                                    return std::nullopt;
                                  }
                                  else
                                  {
                                    return magic_enum::enum_cast<Enum>(v);
                                  } },
                               value))
  {
  }

  template <typename Enum>
  inline param_value enum_param<Enum>::get() const noexcept
  {
    return _value_enum
        ? param_value(std::string(magic_enum::enum_name<Enum>(*_value_enum)))
        : param_null;
  }

  template <typename Enum>
  inline std::optional<Enum> enum_param<Enum>::get_enum() const noexcept
  {
    return _value_enum;
  }

  template <typename Enum>
  inline bool enum_param<Enum>::operator==(const Enum value) const noexcept
  {
    return _value_enum && *_value_enum == value;
  }

  template <typename Enum>
  inline param_error enum_param<Enum>::set(const param_value &value) noexcept
  {
    std::optional<Enum> new_value = std::visit([](auto &&v) -> std::optional<Enum>
                                               {
                            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, null_type>)
                            {
                              return std::nullopt;
                            }
                            else
                            {
                              return magic_enum::enum_cast<Enum>(v);
                            } },
                                               value);

    if (!new_value && !std::holds_alternative<null_type>(value))
    {
      // Only return error if conversion failed (not if explicitly set to null)
      return param_error::invalid_cast;
    }

    _value_enum = new_value;
    _watchable.notify(*this);
    return param_error::ok;
  }

  template <typename Enum>
  inline void enum_param<Enum>::set_enum(const Enum value) noexcept
  {
    _value_enum = value;
    _watchable.notify(*this);
  }

  template <typename Enum>
  inline void enum_param<Enum>::watch(value_changed_func callback, void *ctx)
  {
    _watchable.watch(callback, ctx);
  }

  template <typename Enum>
  inline void enum_param<Enum>::unwatch(value_changed_func callback)
  {
    _watchable.unwatch(callback);
  }

  template <typename Enum>
  param_error try_set_from_param(Enum &target, const enum_param<Enum> &source)
  {
    auto value = source.get_enum();
    if (!value)
    {
      return param_error::invalid_cast;
    }
    target = *value;
    return param_error::ok;
  }

} // namespace cjf

#endif /* C935C86A_5828_4BCC_900C_D24090203B65 */
