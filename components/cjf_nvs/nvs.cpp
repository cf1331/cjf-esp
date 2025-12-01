#include "cjf/nvs.h"
#include <cjf/error_handling.h>
#include <cstring>
#include <nvs_flash.h>

static const char *CJF_NVS = "cjf:nvs";

namespace cjf
{
  nvs_namespace::nvs_namespace(nvs_handle_t handle) noexcept
      : handle_(handle, nvs_handle_deleter())
  {
  }

  esp_err_t nvs_namespace::commit() const noexcept
  {
    return nvs_commit(handle_.get());
  }

  esp_err_t nvs_namespace::erase(const char *key) const noexcept
  {
    return nvs_erase_key(handle_.get(), key);
  }

  esp_err_t nvs_namespace::erase_all() const noexcept
  {
    return nvs_erase_all(handle_.get());
  }

  esp_err_t nvs_namespace::get_blob(const char *key, void *blob, size_t len) const noexcept
  {
    return nvs_get_blob(handle_.get(), key, blob, &len);
  }

  esp_err_t nvs_namespace::set_blob(const char *key, const void *blob, size_t len) const noexcept
  {
    return nvs_set_blob(handle_.get(), key, blob, len);
  }

  esp_err_t nvs_namespace::get_string(const char *key, char *str, size_t len) const noexcept
  {
    return nvs_get_str(handle_.get(), key, str, &len);
  }

  std::expected<std::string, esp_err_t> nvs_namespace::get_string(const char *key) const noexcept
  {
    // First, get the required length
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(handle_.get(), key, nullptr, &required_size);
    if (err != ESP_OK)
    {
      return std::unexpected(err);
    }

    // Allocate buffer and read the string
    std::string result(required_size - 1, '\0'); // -1 because required_size includes null terminator
    err = nvs_get_str(handle_.get(), key, result.data(), &required_size);
    if (err != ESP_OK)
    {
      return std::unexpected(err);
    }
    return result;
  }

  esp_err_t nvs_namespace::set_string(const char *key, const char *str) const noexcept
  {
    return nvs_set_str(handle_.get(), key, str);
  }

  void nvs_namespace::nvs_handle_deleter::operator()(nvs_handle_t handle) const noexcept
  {
    nvs_close(handle);
  }

  std::expected<nvs, esp_err_t> nvs::init() noexcept
  {
    RETURN_UNEXPECTED_ON_ERROR(nvs_flash_init(), CJF_NVS);
    return nvs();
  }

  std::expected<nvs_namespace, esp_err_t> nvs::open(
      const char *namespace_name,
      nvs_open_mode_t mode) const noexcept
  {
    nvs_handle_t handle;
    RETURN_UNEXPECTED_ON_ERROR(nvs_open(namespace_name, mode, &handle), CJF_NVS);
    return nvs_namespace(handle);
  }

  void nvs::deleter::operator()() const noexcept
  {
    LOG_IF_ERROR(nvs_flash_deinit(), CJF_NVS);
  }

} // namespace cjf