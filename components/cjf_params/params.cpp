#include "cjf/params.h"
#include <algorithm>
#include <charconv>
#include <cstring>

namespace cjf
{

  std::expected<std::string_view, param_error>
  to_chars(char *first, char *last, const param_value &value) noexcept
  {
    return std::visit(
        [first, last](auto &&v) -> std::expected<std::string_view, param_error>
        {
          using V = std::decay_t<decltype(v)>;

          if constexpr (std::is_same_v<V, std::monostate>)
          {
            return std::string_view{nullptr, 0};
          }
          else if constexpr (std::is_same_v<V, bool>)
          {
            std::string_view word = v ? "true" : "false";
            if (last - first < static_cast<std::ptrdiff_t>(word.size()))
              return std::unexpected(param_error::out_of_range);
            std::copy(word.begin(), word.end(), first);
            return std::string_view{first, word.size()};
          }
          else if constexpr (std::is_same_v<V, std::string_view>)
          {
            if (last - first < static_cast<std::ptrdiff_t>(v.size()))
              return std::unexpected(param_error::out_of_range);
            std::copy(v.begin(), v.end(), first);
            return std::string_view{first, v.size()};
          }
          else // all numeric types
          {
            auto [ptr, ec] = std::to_chars(first, last, v);
            if (ec == std::errc::value_too_large)
              return std::unexpected(param_error::out_of_range);
            if (ec != std::errc{})
              return std::unexpected(param_error::invalid_cast);
            return std::string_view{first, static_cast<size_t>(ptr - first)};
          }
        },
        value);
  }

} // namespace cjf
