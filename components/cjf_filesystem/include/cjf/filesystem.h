#ifndef DE26C8DB_582C_4D7B_A6A1_F26AFB515416
#define DE26C8DB_582C_4D7B_A6A1_F26AFB515416

#include <esp_err.h>

namespace cjf
{

  esp_err_t mkdirp(const char *path) noexcept;
  esp_err_t mkdirp(const char *begin, const char *end) noexcept;

}

#endif /* DE26C8DB_582C_4D7B_A6A1_F26AFB515416 */
