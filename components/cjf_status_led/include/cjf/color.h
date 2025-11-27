#ifndef C11FB8E8_DE43_48F3_B73E_0CAA48A33440
#define C11FB8E8_DE43_48F3_B73E_0CAA48A33440

#include <cstdint>
#include <variant>

namespace cjf
{
  struct hsv
  {
    uint16_t h; // 0-360
    uint8_t s;  // 0-255
    uint8_t v;  // 0-255

    inline constexpr hsv(uint16_t hue = 0, uint8_t saturation = 0, uint8_t value = 0) noexcept
        : h{hue}, s{saturation}, v{value} {}

    // Multiply operator: scales the brightness (v) by a float in [0, 1]
    inline constexpr hsv operator*(float scale) const {
      return hsv(h, s, static_cast<uint8_t>(v * scale));
    }
  };

  struct rgb
  {
    uint8_t r; // 0-255
    uint8_t g; // 0-255
    uint8_t b; // 0-255

    inline constexpr rgb(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0) noexcept
        : r{red}, g{green}, b{blue} {}

    // Multiply operator: scales each channel by a float in [0, 1]
    inline constexpr rgb operator*(float scale) const {
      return rgb(
        static_cast<uint8_t>(r * scale),
        static_cast<uint8_t>(g * scale),
        static_cast<uint8_t>(b * scale)
      );
    }
  };

  struct rgbw
  {
    uint8_t r; // 0-255
    uint8_t g; // 0-255
    uint8_t b; // 0-255
    uint8_t w; // 0-255

    inline constexpr rgbw(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, uint8_t white = 0) noexcept
        : r{red}, g{green}, b{blue}, w{white} {}

    // Multiply operator: scales each channel by a float in [0, 1]
    inline constexpr rgbw operator*(float scale) const {
      return rgbw(
        static_cast<uint8_t>(r * scale),
        static_cast<uint8_t>(g * scale),
        static_cast<uint8_t>(b * scale),
        static_cast<uint8_t>(w * scale)
      );
    }
  };

  using color = std::variant<hsv, rgb, rgbw>;

  // Multiply operator for color variant: scales the underlying color by a float in [0, 1]
  inline color operator*(const color& c, float scale) {
    return std::visit([scale](const auto& col) -> color {
      return col * scale;
    }, c);
  }

  namespace colors
  {
    constexpr rgb black{0, 0, 0};
    constexpr rgb blue{0, 0, 255};
    constexpr rgb cyan{0, 255, 255};
    constexpr rgb green{0, 255, 0};
    constexpr rgb magenta{255, 0, 255};
    constexpr rgb orange{255, 63, 0};
    constexpr rgb pink{255, 0, 127};
    constexpr rgb purple{127, 0, 255};
    constexpr rgb red{255, 0, 0};
    constexpr rgb white{255, 255, 255};
    constexpr rgb yellow{255, 255, 0};
  }; // namespace cjf::colors

} // namespace cjf

#endif // C11FB8E8_DE43_48F3_B73E_0CAA48A33440
