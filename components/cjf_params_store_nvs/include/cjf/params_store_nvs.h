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
#include <optional>
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
   * cjf::mutable_param<std::string> device_name;
   * store->load<std::string>("device_name", device_name, std::string("ESP32"));
   *
   * // Manually save a parameter
   * device_name.set(std::string("MyDevice"));
   * store->save<std::string>("device_name", device_name);
   * ```
   */
  class params_store_nvs : public std::enable_shared_from_this<params_store_nvs>
  {
  public:
    // Passkey idiom - allows make_shared to call constructor, but external code cannot
    struct passkey {
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
    template <typename T>
    esp_err_t load(const char *key, param &param) noexcept;

    /**
     * @brief Load a parameter with a fallback default value
     * @tparam T Parameter value type
     * @param key NVS key name
     * @param param Parameter to populate
     * @param default_value Value to use if key not found in NVS
     * @return ESP_OK on success (including when default was used)
     */
    template <typename T>
    esp_err_t load(const char *key, param &param, const T &default_value) noexcept;

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
    template <typename T>
    esp_err_t load_and_save_on_change(const char *key, param &param, const T &default_value) noexcept;

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
    template <typename T>
    esp_err_t load_and_save_on_change(const char *key, param &param) noexcept;

    /**
     * @brief Save a parameter value to NVS
     * @tparam T Parameter value type
     * @param key NVS key name
     * @param param Parameter to save
     * @return ESP_OK on success
     */
    template <typename T>
    esp_err_t save(const char *key, param &param) noexcept;

    /**
     * @brief Register auto-save callback for parameter changes
     * @tparam T Parameter value type
     * @param key NVS key name
     * @param param Parameter to watch
     */
    template <typename T>
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
    struct save_on_change_ctx
    {
      std::weak_ptr<params_store_nvs> self;
      std::string key;  // Own the key string to ensure lifetime
    };

    static const char *TAG;
    nvs_namespace ns_;
    std::vector<std::unique_ptr<save_on_change_ctx>> watch_contexts_;  // Auto-cleanup contexts
  };

  // Template implementations

  template <typename T>
  inline esp_err_t params_store_nvs::load(const char *key, param &param) noexcept
  {
    if constexpr (std::is_same_v<T, std::string>)
    {
      auto str = ns_.get_string(key);
      RETURN_ERROR_ON_UNEXPECTED(str, TAG, "Failed to load value for key: %s", key);
      param.set(*str);
      ESP_LOGI(TAG, "%s = %s (loaded)", key, str->c_str());
      return ESP_OK;
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
      T value;
      RETURN_ON_ERROR(ns_.get_blob(key, &value, sizeof(T)), TAG, "Failed to load value for key: %s", key);
      param.set(value);
      ESP_LOGI(TAG, "%s = %s (loaded)", key, param.get_as<std::string>().value_or("").c_str());
      return ESP_OK;
    }
    else
    {
      auto value = ns_.get_item<T>(key);
      RETURN_ERROR_ON_UNEXPECTED(value, TAG, "Failed to load value for key: %s", key);
      param.set(*value);
      ESP_LOGI(TAG, "%s = %s (loaded)", key, param.get_as<std::string>().value_or("").c_str());
      return ESP_OK;
    }
  }

  template <typename T>
  inline esp_err_t params_store_nvs::load(const char *key, param &param, const T &default_value) noexcept
  {
    esp_err_t err = load<T>(key, param);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
      param.set(default_value);
      ESP_LOGI(TAG, "%s = %s (default)", key, param.get_as<std::string>().value_or("").c_str());
      return ESP_OK;
    }
    return err;
  }

  template <typename T>
  inline esp_err_t params_store_nvs::load_and_save_on_change(
      const char *key, param &param, const T &default_value) noexcept
  {
    esp_err_t err = load<T>(key, param, default_value);
    if (err == ESP_OK)
    {
      save_on_change<T>(key, param);
    }
    return err;
  }

  template <typename T>
  inline esp_err_t params_store_nvs::load_and_save_on_change(
      const char *key, param &param) noexcept
  {
    esp_err_t err;

    // If param has a value, use it as the default; otherwise just try to load
    if (param.has_value())
    {
      auto default_value = param.get_as<T>();
      RETURN_ERROR_ON_UNEXPECTED(default_value, TAG,
                                 "Failed to get param value for default for key: %s", key);
      err = load<T>(key, param, *default_value);
    }
    else
    {
      // No default value - only load if key exists in NVS
      err = load<T>(key, param);
      if (err == ESP_ERR_NVS_NOT_FOUND)
      {
        ESP_LOGI(TAG, "%s = null (default)", key);
        err = ESP_OK;  // Not finding the key is OK when there's no default
      }
    }
    RETURN_ON_ERROR(err, TAG, "Failed to load value for key: %s", key);
    save_on_change<T>(key, param);
    return ESP_OK;
  }

  template <typename T>
  inline esp_err_t params_store_nvs::save(const char *key, param &param) noexcept
  {
    if (!param.has_value())
    {
      return erase(key);
    }

    if constexpr (std::is_same_v<T, std::string>)
    {
      auto value = param.get_as<std::string>();

      RETURN_ERROR_ON_UNEXPECTED(value, TAG, "Failed to get string param value for key: %s", key);
      RETURN_ON_ERROR(ns_.set_string(key, value->c_str()), TAG, "Failed to save value for key: %s", key);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
      auto value = param.get_as<T>();
      RETURN_ERROR_ON_UNEXPECTED(value, TAG, "Failed to get floating point param value for key: %s", key);
      RETURN_ON_ERROR(ns_.set_blob(key, &(*value), sizeof(T)), TAG, "Failed to save value for key: %s", key);
    }
    else
    {
      auto value = param.get_as<T>();
      RETURN_ERROR_ON_UNEXPECTED(value, TAG, "Failed to get param value for key: %s", key);
      RETURN_ON_ERROR(ns_.set_item(key, *value), TAG, "Failed to save value for key: %s", key);
    }

    ns_.commit();
    ESP_LOGI(TAG, "%s = %s (saved)", key, param.get_as<std::string>().value_or("").c_str());
    return ESP_OK;
  }

  template <typename T>
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

} // namespace cjf

#endif /* F919C5A5_7D95_4A6D_BA4C_4198B3D21332 */
