#ifndef CC39945C_60FB_4070_AFA3_65F215F92A12
#define CC39945C_60FB_4070_AFA3_65F215F92A12

#include <cjf/scope_guard.h>
#include <esp_err.h>
#include <expected>
#include <experimental/scope>
#include <nvs_flash.h>
#include <nvs_handle.hpp>
#include <string>

namespace cjf
{

  class nvs_namespace
  {
  public:
    nvs_namespace(nvs_handle_t handle) noexcept;

    esp_err_t commit() const noexcept;
    esp_err_t erase(const char *key) const noexcept;
    esp_err_t erase_all() const noexcept;

    template <typename T>
    std::expected<T, esp_err_t> get_item(const char *key) const noexcept
    {
      T value;
      esp_err_t res;

      if constexpr (std::is_same_v<T, uint8_t>)
      {
        res = nvs_get_u8(handle_.get(), key, &value);
      }
      else if constexpr (std::is_same_v<T, int8_t>)
      {
        res = nvs_get_i8(handle_.get(), key, &value);
      }
      else if constexpr (std::is_same_v<T, uint16_t>)
      {
        res = nvs_get_u16(handle_.get(), key, &value);
      }
      else if constexpr (std::is_same_v<T, int16_t>)
      {
        res = nvs_get_i16(handle_.get(), key, &value);
      }
      else if constexpr (std::is_same_v<T, uint32_t>)
      {
        res = nvs_get_u32(handle_.get(), key, &value);
      }
      else if constexpr (std::is_same_v<T, int32_t>)
      {
        res = nvs_get_i32(handle_.get(), key, &value);
      }
      else if constexpr (std::is_same_v<T, uint64_t>)
      {
        res = nvs_get_u64(handle_.get(), key, &value);
      }
      else if constexpr (std::is_same_v<T, int64_t>)
      {
        res = nvs_get_i64(handle_.get(), key, &value);
      }
      else if constexpr (std::is_same_v<T, bool>)
      {
        uint8_t v = 0;
        res = nvs_get_u8(handle_.get(), key, &v);
        value = !!v;
      }
      else if constexpr (std::is_same_v<T, float>)
      {
        uint32_t v = 0;
        res = nvs_get_u32(handle_.get(), key, &v);
        value = std::bit_cast<float>(v);
      }
      else if constexpr (std::is_same_v<T, double>)
      {
        uint64_t v = 0;
        res = nvs_get_u64(handle_.get(), key, &v);
        value = std::bit_cast<double>(v);
      }
      else
      {
        return std::unexpected(ESP_ERR_INVALID_SIZE);
      }

      return (res == ESP_OK) ? std::expected<T, esp_err_t>(value) : std::unexpected(res);
    }

    template <typename T>
    esp_err_t set_item(const char *key, const T &value) const noexcept
    {
      if constexpr (std::is_same_v<T, uint8_t>)
      {
        return nvs_set_u8(handle_.get(), key, value);
      }
      else if constexpr (std::is_same_v<T, int8_t>)
      {
        return nvs_set_i8(handle_.get(), key, value);
      }
      else if constexpr (std::is_same_v<T, uint16_t>)
      {
        return nvs_set_u16(handle_.get(), key, value);
      }
      else if constexpr (std::is_same_v<T, int16_t>)
      {
        return nvs_set_i16(handle_.get(), key, value);
      }
      else if constexpr (std::is_same_v<T, uint32_t>)
      {
        return nvs_set_u32(handle_.get(), key, value);
      }
      else if constexpr (std::is_same_v<T, int32_t>)
      {
        return nvs_set_i32(handle_.get(), key, value);
      }
      else if constexpr (std::is_same_v<T, uint64_t>)
      {
        return nvs_set_u64(handle_.get(), key, value);
      }
      else if constexpr (std::is_same_v<T, int64_t>)
      {
        return nvs_set_i64(handle_.get(), key, value);
      }
      else if constexpr (std::is_same_v<T, bool>)
      {
        uint8_t v = value ? 1 : 0;
        return nvs_set_u8(handle_.get(), key, v);
      }
      else if constexpr (std::is_same_v<T, float>)
      {
        uint32_t v = std::bit_cast<uint32_t>(value);
        return nvs_set_u32(handle_.get(), key, v);
      }
      else if constexpr (std::is_same_v<T, double>)
      {
        uint64_t v = std::bit_cast<uint64_t>(value);
        return nvs_set_u64(handle_.get(), key, v);
      }
      else
      {
        return ESP_ERR_INVALID_SIZE;
      }
    }

    esp_err_t get_blob(const char *key, void *blob, size_t len) const noexcept;
    esp_err_t set_blob(const char *key, const void *blob, size_t len) const noexcept;

    esp_err_t get_string(const char *key, char *str, size_t len) const noexcept;
    std::expected<std::string, esp_err_t> get_string(const char *key) const noexcept;
    esp_err_t set_string(const char *key, const char *str) const noexcept;

    operator nvs_handle_t() const noexcept
    {
      return handle_.get();
    }

  private:
    struct nvs_handle_deleter
    {
      void operator()(nvs_handle_t handle) const noexcept;
    };

    std::experimental::unique_resource<nvs_handle_t, nvs_handle_deleter> handle_;
  };

  class nvs
  {
  public:
    static std::expected<nvs, esp_err_t> init() noexcept;

    std::expected<nvs_namespace, esp_err_t> open(
        const char *namespace_name,
        nvs_open_mode_t mode = NVS_READWRITE) const noexcept;

  private:
    struct deleter
    {
      void operator()() const noexcept;
    };

    cjf::scope_guard<nvs::deleter> scope_guard_;
  };

} // namespace cjf

#endif // CC39945C_60FB_4070_AFA3_65F215F92A12
