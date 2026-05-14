#ifndef F919C5A5_7D95_4A6D_BA4C_4198B3D21332
#define F919C5A5_7D95_4A6D_BA4C_4198B3D21332

/**
 * @file params_store_nvs.h
 * @brief NVS-backed persistent storage for parameters
 */

#include <cjf/error_handling.h>
#include <cjf/nvs.h>
#include <cjf/params.h>
#include <esp_err.h>
#include <esp_log.h>
#include <expected>
#include <memory>
#include <string>
#include <vector>

namespace cjf
{

  /**
   * @brief NVS-backed parameter storage with automatic load/save functionality
   *
   * Provides persistent storage for `cjf::param` objects using ESP-IDF's NVS
   * (Non-Volatile Storage). Supports automatic change tracking and save-on-write
   * functionality.
   *
   * ```cpp
   * // Initialize NVS and create a parameter store
   * auto nvs = std::make_shared<cjf::nvs>(*cjf::nvs::init());
   * auto store = cjf::params_store_nvs::open(nvs, "app_config").value();
   *
   * // Load parameter with default, auto-save on changes
   * cjf::mutable_param<uint32_t> boot_count;
   * store->load_and_save_on_change<uint32_t>("boot_count", boot_count, 0U);
   *
   * // Increment boot count - automatically persists to NVS
   * boot_count.set(boot_count.get_as<uint32_t>().value_or(0) + 1);
   *
   * // Load string parameter with default
   * cjf::mutable_param<std::string_view> device_name;
   * store->load<std::string_view>("device_name", device_name, "ESP32");
   *
   * // Manually save a parameter
   * device_name.set(std::string_view{"MyDevice"});
   * store->save<std::string_view>("device_name", device_name);
   * ```
   */
  class params_store_nvs : public std::enable_shared_from_this<params_store_nvs>
  {
  public:
    constexpr static const size_t DEFAULT_STRING_BUFFER_SIZE = 256;

    // Passkey idiom - allows make_shared to call constructor, but external code cannot
    struct passkey
    {
    private:
      passkey() = default;
      friend class params_store_nvs;
    };

    /**
     * @brief Open an NVS-backed parameter store
     * @param nvs_handle Initialized NVS handle
     * @param namespace_name NVS namespace for parameter storage
     * @return params_store_nvs instance or error code
     */
    static std::expected<std::shared_ptr<params_store_nvs>, esp_err_t> open(
        std::expected<cjf::nvs, esp_err_t> &nvs,
        const char *namespace_name) noexcept;

    /**
     * @brief Load a parameter value from NVS
     * @tparam T Parameter value type (must match stored type)
     * @param key NVS key name
     * @param param Parameter to populate with loaded value
     * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if key doesn't exist
     */
    template <cjf::param_value_type T, size_t BufferSize = DEFAULT_STRING_BUFFER_SIZE>
    esp_err_t load(const char *key, param &param) noexcept;

    /**
     * @brief Load a parameter with a fallback default value
     * @tparam T Parameter value type
     * @param key NVS key name
     * @param param Parameter to populate
     * @param default_value Value to use if key not found in NVS
     * @return ESP_OK on success (including when default was used)
     */
    template <cjf::param_value_type T, size_t BufferSize = DEFAULT_STRING_BUFFER_SIZE>
    esp_err_t load(const char *key, param &param, const std::variant<T, param_null_type> &default_value) noexcept;

    /**
     * @brief Load parameter and register auto-save on value changes
     * @tparam T Parameter value type
     * @param key NVS key name
     * @param param Parameter to load and watch
     * @param default_value Initial value if not in NVS
     * @return ESP_OK on success
     *
     * After calling this, any changes to `param` will automatically persist to NVS.
     */
    template <cjf::param_value_type T, size_t BufferSize = DEFAULT_STRING_BUFFER_SIZE>
    esp_err_t load_and_save_on_change(const char *key, param &param, const std::variant<T, param_null_type> &default_value) noexcept;

    /**
     * @brief Load parameter and register auto-save, using param's current value as default
     * @tparam T Parameter value type
     * @param key NVS key name
     * @param param Parameter to load and watch (uses current value as fallback)
     * @return ESP_OK on success
     *
     * If the param has no value (param_null), the key is loaded from NVS with no default.
     * After calling this, any changes to `param` will automatically persist to NVS.
     */
    template <cjf::param_value_type T, size_t BufferSize = DEFAULT_STRING_BUFFER_SIZE>
    esp_err_t load_and_save_on_change(const char *key, param &param) noexcept;

    /**
     * @brief Save a parameter value to NVS
     * @tparam T Parameter value type
     * @param key NVS key name
     * @param param Parameter to save
     * @return ESP_OK on success
     */
    template <cjf::param_value_type T>
    esp_err_t save(const char *key, param &param) noexcept;

    /**
     * @brief Register auto-save callback for parameter changes
     * @tparam T Parameter value type
     * @param key NVS key name
     * @param param Parameter to watch
     */
    template <cjf::param_value_type T>
    void save_on_change(const char *key, param &param) noexcept;

    /**
     * @brief Erase a key from NVS storage
     * @param key NVS key name
     * @return ESP_OK on success
     */
    esp_err_t erase(const char *key) noexcept;

    // Non-movable, non-copyable (contains self-referential weak_ptrs)
    params_store_nvs(params_store_nvs &&) noexcept = delete;
    params_store_nvs &operator=(params_store_nvs &&) noexcept = delete;
    params_store_nvs(const params_store_nvs &) = delete;
    params_store_nvs &operator=(const params_store_nvs &) = delete;

    // Public constructor for make_shared, but requires passkey
    explicit params_store_nvs(passkey, nvs_namespace &&ns) noexcept;

  private:
    constexpr static const char *ACTION_DEFAULT = "default";
    constexpr static const char *ACTION_LOADED = "loaded";
    constexpr static const char *ACTION_SAVED = "saved";

    constexpr static const char *ERR_FAILED_STRING_CONVERSION = "failed string conversion";
    constexpr static const char *ERR_FAILED_TO_COMMIT_PARAM = "failed to commit param";
    constexpr static const char *ERR_FAILED_TO_ERASE_PARAM = "failed to erase param";
    constexpr static const char *ERR_FAILED_TO_GET_PARAM = "failed to get param";
    constexpr static const char *ERR_FAILED_TO_LOAD_PARAM = "failed to load param";
    constexpr static const char *ERR_FAILED_TO_SAVE_PARAM = "failed to save param";
    constexpr static const char *ERR_FAILED_TO_SET_PARAM = "failed to set param";

    struct save_on_change_ctx
    {
      std::weak_ptr<params_store_nvs> self;
      std::string key; // Own the key string to ensure lifetime
    };

    static const char *TAG;
    nvs_namespace ns_;
    std::vector<std::unique_ptr<save_on_change_ctx>> watch_contexts_; // Auto-cleanup contexts

    template <cjf::param_value_type T>
    void log_param_(const char *key, const T &value, const char *action) const noexcept;
    void log_param_error_(const char *key, esp_err_t err, const char *msg) const noexcept;
    void log_param_error_(const char *key, param_error err, const char *msg) const noexcept;
  };

  // Template implementations

  template <cjf::param_value_type T, size_t BufferSize>
  inline esp_err_t params_store_nvs::load(const char *key, param &param) noexcept
  {
    if constexpr (std::is_same_v<T, std::string_view>)
    {
      auto blob_size = ns_.get_blob_size(key);
      if (!blob_size) return blob_size.error();
      std::array<char, BufferSize> buf;
      size_t max_size = std::min(blob_size.value(), buf.size());
      if (auto res = ns_.get_blob(key, buf.data(), max_size); res != ESP_OK) return res;
      std::string_view value{buf.data(), max_size};
      if (auto res = param.set(value); res != param_error::ok)
      {
        log_param_error_(key, res, ERR_FAILED_TO_SET_PARAM);
        return ESP_FAIL;
      }
      log_param_(key, value, ACTION_LOADED);
      return ESP_OK;
    }
    else
    {
      auto value = ns_.get_item<T>(key);
      if (!value) return value.error();
      if (auto res = param.set(*value); res != param_error::ok)
      {
        log_param_error_(key, res, ERR_FAILED_TO_SET_PARAM);
        return ESP_FAIL;
      }
      log_param_(key, *value, ACTION_LOADED);
      return ESP_OK;
    }
  }

  template <cjf::param_value_type T, size_t BufferSize>
  inline esp_err_t params_store_nvs::load(const char *key, param &param, const std::variant<T, param_null_type> &default_value) noexcept
  {
    esp_err_t err = load<T>(key, param);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
      param.set(default_value);
      log_param_(key, default_value, ACTION_DEFAULT);
      err = ESP_OK; // Not finding the key is OK when we have a default
    }
    return err;
  }

  template <cjf::param_value_type T, size_t BufferSize>
  inline esp_err_t params_store_nvs::load_and_save_on_change(
      const char *key, param &param) noexcept
  {
    esp_err_t err = load<T>(key, param);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
      log_param_error_(key, err, ERR_FAILED_TO_LOAD_PARAM);
      return err;
    }
    save_on_change<T>(key, param);
    return ESP_OK;
  }

  template <cjf::param_value_type T, size_t BufferSize>
  inline esp_err_t params_store_nvs::load_and_save_on_change(
      const char *key, param &param, const std::variant<T, param_null_type> &default_value) noexcept
  {
    esp_err_t err = load<T>(key, param, default_value);
    if (err != ESP_OK)
    {
      log_param_error_(key, err, ERR_FAILED_TO_LOAD_PARAM);
      return err;
    }
    save_on_change<T>(key, param);
    return ESP_OK;
  }

  template <cjf::param_value_type T>
  inline esp_err_t params_store_nvs::save(const char *key, param &param) noexcept
  {
    if (!param.has_value())
    {
      esp_err_t err = erase(key);
      if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
      {
        log_param_error_(key, err, ERR_FAILED_TO_ERASE_PARAM);
        return err;
      }
      ESP_LOGI(TAG, "%s = <erased> (%s)", key, ACTION_SAVED);
      return ESP_OK;
    }

    auto value = param.get_as<T>();
    if (!value)
    {
      log_param_error_(key, value.error(), ERR_FAILED_TO_GET_PARAM);
      return ESP_FAIL;
    }

    if constexpr (std::is_same_v<T, std::string_view>)
    {
      if (auto res = ns_.set_blob(key, value->data(), value->size()); res != ESP_OK)
      {
        log_param_error_(key, res, ERR_FAILED_TO_SAVE_PARAM);
        return res;
      }
    }
    else
    {
      if (auto res = ns_.set_item(key, *value); res != ESP_OK)
      {
        log_param_error_(key, res, ERR_FAILED_TO_SAVE_PARAM);
        return res;
      }
    }

    if (esp_err_t res = ns_.commit(); res != ESP_OK)
    {
      log_param_error_(key, res, ERR_FAILED_TO_COMMIT_PARAM);
      return res;
    }

    log_param_(key, *value, ACTION_SAVED);
    return ESP_OK;
  }

  template <cjf::param_value_type T>
  inline void params_store_nvs::save_on_change(const char *key, param &param) noexcept
  {
    // Use weak_ptr to avoid dangling pointer if params_store_nvs is destroyed
    auto &ctx = watch_contexts_.emplace_back(
        std::make_unique<save_on_change_ctx>(weak_from_this(), std::string(key)));

    param.watch(
        [](cjf::param &p, void *context)
        {
          auto ctx = reinterpret_cast<save_on_change_ctx *>(context);
          // Try to lock the weak_ptr - if store still exists, save the value
          if (auto store = ctx->self.lock())
          {
            store->save<T>(ctx->key.c_str(), p);
          }
          else
          {
            ESP_LOGW(TAG, "params_store_nvs no longer exists, cannot save %s", ctx->key.c_str());
          }
        },
        ctx.get());
  }

  template <cjf::param_value_type T>
  inline void params_store_nvs::log_param_(const char *key, const T &value, const char *action) const noexcept
  {
    if constexpr (std::is_same_v<T, std::string_view>)
    {
      ESP_LOGI(TAG, "%s = \"%.*s\" (%s)", key, value.size(), value.data(), action);
    }
    else
    {
      std::array<char, 32> buf;
      auto value_str = cjf::to_chars(buf.begin(), buf.end(), value);
      if (value_str.has_value())
      {
        ESP_LOGI(TAG, "%s = %.*s (%s)", key, value_str->size(), value_str->data(), action);
      }
      else
      {
        log_param_error_(key, value_str.error(), ERR_FAILED_STRING_CONVERSION);
      }
    }
  }

  inline void params_store_nvs::log_param_error_(const char *key, esp_err_t err, const char *msg) const noexcept
  {
    ESP_LOGE(TAG, "%s = <%s: %s>", key, msg, esp_err_to_name(err));
  }

  inline void params_store_nvs::log_param_error_(const char *key, param_error err, const char *msg) const noexcept
  {
    ESP_LOGE(TAG, "%s = <%s: %s>", key, msg, param_error_to_name(err));
  }

} // namespace cjf

#endif /* F919C5A5_7D95_4A6D_BA4C_4198B3D21332 */
