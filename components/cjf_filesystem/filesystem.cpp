#include "cjf/filesystem.h"
#include <sys/stat.h>

namespace cjf
{
  static size_t const MAX_PATH_LENGTH = 256;

  esp_err_t mkdirp(const char *path) noexcept
  {
    char dirname[MAX_PATH_LENGTH] = {0};
    const char *c = path;
    char *d = dirname;

    while (true)
    {
      if (*c == '/' || *c == '\0')
      {
        *d = '\0';
        if (d != dirname)
        {
          mkdir(dirname, 0755); // ignore EEXIST
        }
        if (*c == '\0') break;
        if (d >= dirname + sizeof(dirname) - 1) return ESP_ERR_INVALID_SIZE;
        *d++ = '/';
      }
      else
      {
        if (d >= dirname + sizeof(dirname) - 1) return ESP_ERR_INVALID_SIZE;
        *d++ = *c;
      }
      ++c;
    }
    return ESP_OK;
  }

  esp_err_t mkdirp(const char *begin, const char *end) noexcept
  {
    char dirname[MAX_PATH_LENGTH] = {0};
    const char *c = begin;
    char *d = dirname;

    if (end - begin >= sizeof(dirname))
    {
      return ESP_ERR_INVALID_SIZE;
    }

    while (c <= end)
    {
      if (*c == '/' || c == end || *c == '\0')
      {
        *d = '\0';
        if (d != dirname)
        {
          mkdir(dirname, 0755); // ignore EEXIST
        }
        if (c == end || *c == '\0') break;
        *d++ = '/';
      }
      else
      {
        *d++ = *c;
      }
      ++c;
    }
    return ESP_OK;
  }
} // namespace cjf
