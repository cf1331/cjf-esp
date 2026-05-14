#ifndef C0697E51_E12E_4C4B_B733_789B1927B87F
#define C0697E51_E12E_4C4B_B733_789B1927B87F

#include <algorithm>
#include <cctype>
#include <concepts>
#include <string>
#include <string_view>
#include <utility>

namespace cjf
{
  [[nodiscard]] constexpr char ascii_tolower(char ch) noexcept
  {
    return (ch >= 'A' && ch <= 'Z') ? (ch | 0x20) : ch;
  }

  [[nodiscard]] constexpr char ascii_toupper(char ch) noexcept
  {
    return (ch >= 'a' && ch <= 'z') ? (ch & ~0x20) : ch;
  }

  template <typename T, typename U>
    requires std::convertible_to<T, std::string_view> &&
      std::convertible_to<U, std::string_view>
  [[nodiscard]] constexpr bool case_insensitive_equal(T &&a, U &&b) noexcept
  {
    std::string_view sv_a{std::forward<T>(a)};
    std::string_view sv_b{std::forward<U>(b)};

    if (sv_a.size() != sv_b.size()) return false;

    for (size_t i = 0; i < sv_a.size(); ++i)
    {
      if (ascii_tolower(sv_a[i]) != ascii_tolower(sv_b[i]))
      {
        return false;
      }
    }
    return true;
  }

  std::string to_lower(const std::string &s);
  std::string to_upper(const std::string &s);
} // namespace cjf

#endif /* C0697E51_E12E_4C4B_B733_789B1927B87F */
