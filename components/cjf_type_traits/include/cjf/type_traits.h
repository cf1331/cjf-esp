#ifndef D927AE6B_1547_451D_A086_488305B670E1
#define D927AE6B_1547_451D_A086_488305B670E1

#include <variant>

namespace cjf
{
  template <class>
  inline constexpr bool always_false_v = false;

  template <typename T, typename Variant>
  struct is_variant_alternative : std::false_type
  {
  };

  template <typename T, typename... Ts>
  struct is_variant_alternative<T, std::variant<Ts...>>
      : std::disjunction<std::is_same<T, Ts>...>
  {
  };
} // namespace cjf

#endif /* D927AE6B_1547_451D_A086_488305B670E1 */
