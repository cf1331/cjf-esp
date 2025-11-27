#ifndef E650A9D3_5209_4DC8_84ED_352A730E622C
#define E650A9D3_5209_4DC8_84ED_352A730E622C

#include <algorithm>

namespace cjf
{
  using easing_func = float (*)(float);

  namespace easing
  {

    inline constexpr float linear(float t)
    {
      return std::clamp(t, 0.0f, 1.0f);
    }

    inline constexpr float quad_in(float t)
    {
      return std::clamp(t * t, 0.0f, 1.0f);
    }

    inline constexpr float quad_out(float t)
    {
      return std::clamp(t * (2 - t), 0.0f, 1.0f);
    }

    inline constexpr float quad_in_out(float t)
    {
      return (t < 0.5f)
          ? std::clamp(2 * t * t, 0.0f, 1.0f)
          : std::clamp(-2 * t * t + 4 * t - 1, 0.0f, 1.0f);
    }

  } // namespace easing
} // namespace cjf

#endif // E650A9D3_5209_4DC8_84ED_352A730E622C
