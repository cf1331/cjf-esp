#include "cjf/string.h"
#include <algorithm>
#include <cctype>

namespace cjf
{
  std::string to_lower(const std::string &s)
  {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
  }

  std::string to_upper(const std::string &s)
  {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
  }

} // namespace cjf
