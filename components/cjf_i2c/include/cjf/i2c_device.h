#ifndef BA9FD786_753C_4ACA_921E_CCEFE490D913
#define BA9FD786_753C_4ACA_921E_CCEFE490D913

/**
 * @file i2c_device.h
 * @brief I2C device abstraction classes for ESP-IDF
 *
 * This file provides C++ wrapper classes for ESP-IDF I2C device functionality,
 * including base device operations and register-based device operations.
 */

#include "cjf/i2c_master.h"
#include <cjf/reg.h>
#include <driver/i2c_master.h>
#include <esp_log.h>
#include <expected>

namespace cjf
{
  /**
   * @brief Base class for I2C device operations
   *
   * Provides a C++ wrapper around ESP-IDF I2C device functionality with RAII
   * management of device handles. This class serves as a base for I2C devices.
   */
  class i2c_device
  {
  public:
    /**
     * @brief Custom deleter for I2C device handles
     */
    struct handle_deleter
    {
      handle_deleter() = default;
      void operator()(i2c_master_dev_handle_t device) const noexcept;
    };

    /**
     * @brief Type alias for I2C device handle with custom deleter to handle cleanup
     */
    using handle_type = std::unique_ptr<i2c_master_dev_t, handle_deleter>;

    // Move-only semantics: Each `i2c_device` represents exclusive ownership of an ESP-IDF
    // `i2c_master_dev_handle_t`. Copying would create multiple owners of the same handle,
    // leading to double-deletion and undefined behavior when devices are destroyed.
    i2c_device(const i2c_device &) = delete;
    i2c_device &operator=(const i2c_device &) = delete;
    i2c_device(i2c_device &&) noexcept = default;
    i2c_device &operator=(i2c_device &&) noexcept = default;

    /**
     * @brief Implicit conversion to `i2c_master_dev_handle_t`
     * @return The underlying ESP-IDF device handle
     */
    operator i2c_master_dev_handle_t() const noexcept;

    /**
     * @brief Get the I2C address of this device
     * @return The I2C address (7-bit or 10-bit)
     */
    uint16_t address() const noexcept;

    /**
     * @brief Probe the device to check if it's present on the bus
     * @param timeout_ms Timeout in milliseconds for the probe operation (-1 to wait forever)
     * @return `ESP_OK` if device responds, error code otherwise
     */
    esp_err_t probe(int32_t timeout_ms) const;

  protected:
    /// Log tag for I2C device operations
    static constexpr const char *CJF_I2C_DEVICE = "cjf:i2c_device";

    /// Reference to the I2C master bus this device is connected to
    const i2c_master *master_;

    /**
     * @brief Add a device to the I2C bus
     * @param master The I2C master bus
     * @param config Device configuration
     * @return Expected containing `unique_ptr` to device handle if successful or an error code
     */
    static std::expected<handle_type, esp_err_t> add_to_bus(
        const i2c_master &master,
        const i2c_device_config_t &config);

    /**
     * @brief Protected constructor for derived classes
     * @param master Reference to the I2C master bus
     * @param address I2C address of the device (7-bit or 10-bit)
     * @param handle Unique pointer to the device handle
     */
    explicit i2c_device(const i2c_master &master, uint16_t address, handle_type &&handle) noexcept;

    /**
     * @brief Read data from the device
     * @param buffer Buffer to store read data
     * @param buffer_size Size of the buffer
     * @param timeout_ms Timeout in milliseconds
     * @return `ESP_OK` on success, error code otherwise
     */
    esp_err_t read(uint8_t *buffer, const size_t buffer_size, const int32_t timeout_ms) const noexcept;

    /**
     * @brief Write data to the device
     * @param buf Data buffer to write
     * @param size Size of data to write
     * @param timeout_ms Timeout in milliseconds
     * @return `ESP_OK` on success, error code otherwise
     */
    esp_err_t write(const uint8_t *buf, const size_t size, const int32_t timeout_ms) const noexcept;

  private:
    /// I2C address of this device (7-bit or 10-bit)
    uint16_t address_;

    /// Managed handle to the ESP-IDF device
    handle_type handle_;
  };

  /**
   * @brief I2C device base class for devices with register-based operations
   *
   * Extends `i2c_device` to provide type-safe register read/write operations.
   *
   * @tparam Device The specific device type (CRTP)
   * @tparam RegAddressType Type used for register addresses (e.g., `uint8_t`, `uint16_t`)
   */
  template <class Device, typename RegAddressType>
  class i2c_device_with_registers : public i2c_device, public has_registers<Device, RegAddressType>
  {
  private:
    static constexpr const char *I2C_DEVICE_TAG = "cjf:i2c_device";

  protected:
    using i2c_device::i2c_device;

    using reg_address_type = RegAddressType;

    /// Read-only register type alias
    template <RegAddressType RegAddress, typename ValueType>
    using reg_ro = typename has_registers<Device, RegAddressType>::template reg_ro<RegAddress, ValueType>;

    /// Read-write register type alias
    template <RegAddressType RegAddress, typename ValueType>
    using reg_rw = typename has_registers<Device, RegAddressType>::template reg_rw<RegAddress, ValueType>;

    /// Write-only register type alias
    template <RegAddressType RegAddress, typename ValueType>
    using reg_wo = typename has_registers<Device, RegAddressType>::template reg_wo<RegAddress, ValueType>;

    /**
     * @brief Read from a read-only register
     * @tparam RegAddress Compile-time register address
     * @tparam ValueType Type of the register value
     * @param reg The register to read from
     * @param timeout_ms Timeout in milliseconds (-1 to wait forever)
     * @return Expected containing the register value or error code
     */
    template <RegAddressType RegAddress, typename ValueType>
    std::expected<ValueType, esp_err_t> read(
        const reg_ro<RegAddress, ValueType> reg,
        const int32_t timeout_ms = -1) const
    {
      return read<ValueType>(reg.address, timeout_ms);
    }

    /**
     * @brief Read from a read-write register
     * @tparam RegAddress Compile-time register address
     * @tparam ValueType Type of the register value
     * @param reg The register to read from
     * @param timeout_ms Timeout in milliseconds (-1 to wait forever)
     * @return Expected containing the register value or error code
     */
    template <RegAddressType RegAddress, typename ValueType>
    std::expected<ValueType, esp_err_t> read(
        const reg_rw<RegAddress, ValueType> reg,
        const int32_t timeout_ms = -1) const
    {
      return read<ValueType>(reg.address, timeout_ms);
    }

    /**
     * @brief Read from a register by address
     * @tparam ValueType Type of the register value (can be native type or register type)
     * @param reg_address The register address to read from
     * @param timeout_ms Timeout in milliseconds (-1 to wait forever)
     * @return Expected containing the register value or error code
     */
    template <typename ValueType>
    std::expected<ValueType, esp_err_t> read(
        const RegAddressType reg_address,
        const int32_t timeout_ms) const
    {
      ValueType value;
      esp_err_t err;
      ESP_LOGD(I2C_DEVICE_TAG, "Reading from register: 0x%x", reg_address);
      if constexpr (requires { value.data(); value.size(); })
      {
        // ValueType has data() and size() methods (register types)
        err = i2c_master_transmit_receive(
            *this,
            reinterpret_cast<const uint8_t *>(&reg_address),
            sizeof(RegAddressType),
            reinterpret_cast<uint8_t *>(value.data()),
            value.size(),
            timeout_ms);
        ESP_LOG_BUFFER_HEX_LEVEL(I2C_DEVICE_TAG, value.data(), value.size(), ESP_LOG_DEBUG);
      }
      else
      {
        // ValueType is a native type
        err = i2c_master_transmit_receive(
            *this,
            reinterpret_cast<const uint8_t *>(&reg_address),
            sizeof(RegAddressType),
            reinterpret_cast<uint8_t *>(&value),
            sizeof(ValueType),
            timeout_ms);
        ESP_LOG_BUFFER_HEX_LEVEL(I2C_DEVICE_TAG, &value, sizeof(ValueType), ESP_LOG_DEBUG);
      }
      RETURN_UNEXPECTED_ON_ERROR(err, CJF_I2C_DEVICE);
      return value;
    }

    /**
     * @brief Write to a write-only register
     * @tparam RegAddress Compile-time register address
     * @tparam ValueType Type of the register value
     * @param reg The register to write to
     * @param value The value to write
     * @param timeout_ms Timeout in milliseconds (-1 to wait forever)
     * @return `ESP_OK` on success, error code otherwise
     */
    template <RegAddressType RegAddress, typename ValueType>
    esp_err_t write(
        const reg_wo<RegAddress, ValueType> reg,
        const auto& value,
        const int32_t timeout_ms = -1) const
      requires std::constructible_from<ValueType, std::decay_t<decltype(value)>>
    {
      return write<ValueType>(reg.address, ValueType(value), timeout_ms);
    }

    /**
     * @brief Write to a read-write register
     * @tparam RegAddress Compile-time register address
     * @tparam ValueType Type of the register value
     * @param reg The register to write to
     * @param value The value to write
     * @param timeout_ms Timeout in milliseconds (-1 to wait forever)
     * @return `ESP_OK` on success, error code otherwise
     */
    template <RegAddressType RegAddress, typename ValueType>
    esp_err_t write(
        const reg_rw<RegAddress, ValueType> reg,
        const auto& value,
        const int32_t timeout_ms = -1) const
      requires std::constructible_from<ValueType, std::decay_t<decltype(value)>>
    {
      return write<ValueType>(reg.address, ValueType(value), timeout_ms);
    }

    /**
     * @brief Write to a read-write register with bit masking
     * @tparam RegAddress Compile-time register address
     * @tparam ValueType Type of the register value
     * @param reg The register to write to
     * @param mask Bit mask indicating which bits to modify (1 = modify, 0 = preserve)
     * @param value The value to write (only bits set in mask will be applied)
     * @param timeout_ms Timeout in milliseconds (-1 to wait forever)
     * @return `ESP_OK` on success, error code otherwise
     * @note This performs a read-modify-write operation: reads current value,
     *       clears bits specified by mask, then sets bits from value where mask is 1
     */
    template <RegAddressType RegAddress, typename ValueType>
    esp_err_t write(
        const reg_rw<RegAddress, ValueType> reg,
        const auto& mask,
        const auto& value,
        const int32_t timeout_ms = -1) const
      requires std::constructible_from<ValueType, std::decay_t<decltype(mask)>> &&
               std::constructible_from<ValueType, std::decay_t<decltype(value)>>
    {
      return write<ValueType>(reg.address, ValueType(mask), ValueType(value), timeout_ms);
    }

    /**
     * @brief Write to a register by address
     * @tparam RegAddress Compile-time register address
     * @tparam ValueType Type of the register value
     * @param reg_address The register address to write to
     * @param value The value to write
     * @param timeout_ms Timeout in milliseconds (-1 to wait forever)
     * @return `ESP_OK` on success, error code otherwise
     */
    template <typename ValueType>
    esp_err_t write(
        RegAddressType reg_address,
        ValueType value,
        const int32_t timeout_ms) const
    {
      i2c_master_transmit_multi_buffer_info_t buffers[] = {
          {.write_buffer = reinterpret_cast<uint8_t *>(&reg_address),
           .buffer_size = sizeof(reg_address)},
          {.write_buffer = reinterpret_cast<uint8_t *>(&value),
           .buffer_size = sizeof(ValueType)}};
      esp_err_t err = i2c_master_multi_buffer_transmit(
          *this,
          buffers,
          sizeof(buffers) / sizeof(i2c_master_transmit_multi_buffer_info_t),
          timeout_ms);
      RETURN_ON_ERROR(err, CJF_I2C_DEVICE);
      return ESP_OK;
    }

    /**
     * @brief Write to a register by address with bit masking
     * @tparam ValueType Type of the register value
     * @param reg_address The register address to write to
     * @param mask Bit mask indicating which bits to modify (1 = modify, 0 = preserve)
     * @param value The value to write (only bits set in mask will be applied)
     * @param timeout_ms Timeout in milliseconds (-1 to wait forever)
     * @return `ESP_OK` on success, error code otherwise
     * @note This performs a read-modify-write operation: reads current value,
     *       clears bits specified by mask, then sets bits from value where mask is 1
     */
    template <typename ValueType>
    esp_err_t write(
        const RegAddressType reg_address,
        const ValueType mask,
        const ValueType value,
        const int32_t timeout_ms) const
    {
      auto result = read<ValueType>(reg_address, timeout_ms)
                        .transform([&](auto current_value)
                                   {
          auto new_value = (current_value & ~mask) | (value & mask);
          // ESP_LOGI(I2C_DEVICE_TAG, "Existing value:");
          // ESP_LOG_BUFFER_HEX(I2C_DEVICE_TAG, &current_value, sizeof(ValueType));
          // ESP_LOGI(I2C_DEVICE_TAG, "Mask:");
          // ESP_LOG_BUFFER_HEX(I2C_DEVICE_TAG, &mask, sizeof(ValueType));
          // ESP_LOGI(I2C_DEVICE_TAG, "Value:");
          // ESP_LOG_BUFFER_HEX(I2C_DEVICE_TAG, &value, sizeof(ValueType));
          // ESP_LOGI(I2C_DEVICE_TAG, "New value:");
          // ESP_LOG_BUFFER_HEX(I2C_DEVICE_TAG, &new_value, sizeof(ValueType));
          return write<ValueType>(reg_address, new_value, timeout_ms); });
      return result.has_value() ? result.value() : result.error();
    }
  };

} // namespace cjf

#endif /* BA9FD786_753C_4ACA_921E_CCEFE490D913 */
