#ifndef CC492F62_D615_499F_A2E3_B6817D25FFC6
#define CC492F62_D615_499F_A2E3_B6817D25FFC6

#include <cstdlib>
#include <memory>

namespace cjf
{
  template <typename T>
  using unique_c_ptr = std::unique_ptr<T, decltype([](void *p) { free(p); })>;
}

#endif /* CC492F62_D615_499F_A2E3_B6817D25FFC6 */
