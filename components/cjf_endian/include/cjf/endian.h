#ifndef A5FD0473_0358_4683_A294_FBB4E5B3FA61
#define A5FD0473_0358_4683_A294_FBB4E5B3FA61

/**
 * @file endian.h
 * @brief Endian-aware integer types for cross-platform binary data handling
 *
 * This file provides template classes for handling integers with specific endianness,
 * enabling safe and efficient handling of binary data formats, network protocols,
 * and file formats that require specific byte ordering.
 */

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace cjf
{
  /**
   * @brief Template class for endian-aware integer values
   *
   * Provides a wrapper for integer types that stores values in a specific endianness,
   * automatically handling byte-order conversion when converting to/from native types.
   * This is the general implementation that works with any endianness and size.
   *
   * @tparam Endian The target endianness (std::endian::big or std::endian::little)
   * @tparam NativeType The native integer type that values can be converted to/from
   * @tparam Size The size in bytes for the stored value (defaults to sizeof(NativeType))
   */
  template <std::endian Endian, typename NativeType, size_t Size = sizeof(NativeType)>
  struct endian_value
  {
    /// Compile-time safety checks to ensure valid template parameters
    static_assert(Size > 0, "Size must be greater than 0");
    static_assert(Size <= 8, "Size cannot be larger than largest native type (<=8 bytes)");
    static_assert(std::is_integral_v<NativeType>, "NativeType must be an integral type");
    static_assert(Endian == std::endian::big || Endian == std::endian::little,
                  "Endian must be big or little endian");

  private:
    /// Raw byte storage for the value in the specified endianness
    uint8_t data_[Size];

    /**
     * @brief Check if the stored value represents a negative number
     * @return true if NativeType is signed and the value is negative, false otherwise
     */
    constexpr bool _is_negative() const noexcept
    {
      if constexpr (Size == 0 || !std::is_signed_v<NativeType>)
      {
        return false;
      }
      else if constexpr (Endian == std::endian::big)
      {
        return (data_[0] & 0x80) != 0;
      }
      else
      {
        return (data_[Size - 1] & 0x80) != 0;
      }
    }

  public:
    /// Type alias for the native integer type that values can be converted to/from
    using native_type = NativeType;

    /**
     * @brief Default constructor - initializes to zero
     */
    endian_value() = default;

    /**
     * @brief Constructor from native value
     * @param value The native value to store in the specified endianness
     */
    constexpr endian_value(NativeType value) noexcept
    {
      // Store the value in the native endianness first
      if constexpr (Size > sizeof(NativeType))
      {
        // Handle sign extension for smaller signed types
        const uint8_t extension = value < 0 ? 0xFF : 0x00;
        std::memset(data_, extension, Size);
        std::memcpy(data_, &value, sizeof(NativeType));
      }
      else
      {
        std::memcpy(data_, &value, Size);
      }
      // Reverse the bytes if the target endianness is different from native
      if constexpr (Endian != std::endian::native)
      {
        std::reverse(data_, data_ + Size);
      }
    }

    /**
     * @brief Get mutable pointer to the raw byte data
     * @return Pointer to the internal byte array
     */
    uint8_t *data() noexcept
    {
      return data_;
    }

    /**
     * @brief Get const pointer to the raw byte data
     * @return Const pointer to the internal byte array
     */
    const uint8_t *data() const noexcept
    {
      return data_;
    }

    /**
     * @brief Get the size in bytes of the value
     * @return The size in bytes
     */
    constexpr size_t size() const noexcept
    {
      return Size;
    }

    /**
     * @brief Convert the value to native type
     * @return The value in native endianness and type
     */
    constexpr NativeType to_native() const noexcept
    {
      NativeType value = _is_negative() ? static_cast<NativeType>(-1) : 0;
      if constexpr (Endian == std::endian::native)
      {
        constexpr size_t size = Size <= sizeof(NativeType) ? Size : sizeof(NativeType);
        std::memcpy(&value, data_, size);
      }
      else
      {
        constexpr uint32_t offset = (Size <= sizeof(NativeType)) ? 0 : (Size - sizeof(NativeType));
        std::reverse_copy(data_ + offset, data_ + Size, reinterpret_cast<uint8_t *>(&value));
      }
      return value;
    }

    /**
     * @brief Implicit conversion to native type
     * @return The value converted to native type
     */
    constexpr operator NativeType() const noexcept
    {
      return to_native();
    }

    /**
     * @brief Assignment from native type
     * @param value The native value to assign
     * @return Reference to this object
     */
    constexpr endian_value &operator=(NativeType value) noexcept
    {
      *this = endian_value(value);
      return *this;
    }

    /**
     * @brief Equality comparison with another endian_value
     * @param other The other endian_value to compare with
     * @return true if values are equal, false otherwise
     */
    constexpr bool operator==(const endian_value &other) const noexcept
    {
      // Byte-by-byte comparison for general case
      for (size_t i = 0; i < Size; ++i)
      {
        if (data_[i] != other.data_[i])
          return false;
      }
      return true;
    }

    /**
     * @brief Inequality comparison with another endian_value
     * @param other The other endian_value to compare with
     * @return true if values are not equal, false otherwise
     */
    constexpr bool operator!=(const endian_value &other) const noexcept
    {
      return !(*this == other);
    }

    /**
     * @brief Equality comparison with native type
     * @param value The native value to compare with
     * @return true if values are equal, false otherwise
     */
    constexpr bool operator==(NativeType value) const noexcept
    {
      return to_native() == value;
    }

    /**
     * @brief Inequality comparison with native type
     * @param value The native value to compare with
     * @return true if values are not equal, false otherwise
     */
    constexpr bool operator!=(NativeType value) const noexcept
    {
      return to_native() != value;
    }

    /**
     * @brief Bitwise OR with another endian_value
     * @param other The other endian_value to OR with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator|(const endian_value &other) const noexcept
    {
      endian_value result;
      for (size_t i = 0; i < Size; ++i)
      {
        result.data_[i] = data_[i] | other.data_[i];
      }
      return result;
    }

    /**
     * @brief Bitwise OR with native type
     * @param value The native value to OR with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator|(NativeType value) const
    {
      endian_value other(value);
      return *this | other;
    }

    /**
     * @brief Bitwise OR assignment with another endian_value
     * @param other The other endian_value to OR with
     * @return Reference to this object
     */
    endian_value &operator|=(const endian_value &other) noexcept
    {
      for (size_t i = 0; i < Size; ++i)
      {
        data_[i] |= other.data_[i];
      }
      return *this;
    }

    /**
     * @brief Bitwise OR assignment with native type
     * @param value The native value to OR with
     * @return Reference to this object
     */
    endian_value &operator|=(NativeType value)
    {
      endian_value other(value);
      return *this |= other;
    }

    /**
     * @brief Bitwise AND with another endian_value
     * @param other The other endian_value to AND with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator&(const endian_value &other) const noexcept
    {
      endian_value result;
      for (size_t i = 0; i < Size; ++i)
      {
        result.data_[i] = data_[i] & other.data_[i];
      }
      return result;
    }

    /**
     * @brief Bitwise AND with native type
     * @param value The native value to AND with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator&(NativeType value) const
    {
      endian_value other(value);
      return *this & other;
    }

    /**
     * @brief Bitwise AND assignment with another endian_value
     * @param other The other endian_value to AND with
     * @return Reference to this object
     */
    endian_value &operator&=(const endian_value &other) noexcept
    {
      for (size_t i = 0; i < Size; ++i)
      {
        data_[i] &= other.data_[i];
      }
      return *this;
    }

    /**
     * @brief Bitwise AND assignment with native type
     * @param value The native value to AND with
     * @return Reference to this object
     */
    endian_value &operator&=(NativeType value)
    {
      endian_value other(value);
      return *this &= other;
    }

    /**
     * @brief Bitwise XOR with another endian_value
     * @param other The other endian_value to XOR with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator^(const endian_value &other) const noexcept
    {
      endian_value result;
      for (size_t i = 0; i < Size; ++i)
      {
        result.data_[i] = data_[i] ^ other.data_[i];
      }
      return result;
    }

    /**
     * @brief Bitwise XOR with native type
     * @param value The native value to XOR with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator^(NativeType value) const
    {
      endian_value other(value);
      return *this ^ other;
    }

    /**
     * @brief Bitwise XOR assignment with another endian_value
     * @param other The other endian_value to XOR with
     * @return Reference to this object
     */
    endian_value &operator^=(const endian_value &other) noexcept
    {
      for (size_t i = 0; i < Size; ++i)
      {
        data_[i] ^= other.data_[i];
      }
      return *this;
    }

    /**
     * @brief Bitwise XOR assignment with native type
     * @param value The native value to XOR with
     * @return Reference to this object
     */
    endian_value &operator^=(NativeType value)
    {
      endian_value other(value);
      return *this ^= other;
    }

    /**
     * @brief Bitwise NOT operation
     * @return New value with the same endianess and size with all bits inverted
     */
    endian_value operator~() const noexcept
    {
      endian_value result;
      for (size_t i = 0; i < Size; ++i)
      {
        result.data_[i] = ~data_[i];
      }
      return result;
    }

    /**
     * @brief Left shift operation
     * @param shift Number of bits to shift left
     * @return New value with the same endianess and size containing the shifted result
     */
    endian_value operator<<(int shift) const
    {
      return endian_value(to_native() << shift);
    }

    /**
     * @brief Left shift assignment
     * @param shift Number of bits to shift left
     * @return Reference to this object
     */
    endian_value &operator<<=(int shift)
    {
      *this = endian_value(to_native() << shift);
      return *this;
    }

    /**
     * @brief Right shift operation
     * @param shift Number of bits to shift right
     * @return New value with the same endianess and size containing the shifted result
     */
    endian_value operator>>(int shift) const
    {
      return endian_value(to_native() >> shift);
    }

    /**
     * @brief Right shift assignment
     * @param shift Number of bits to shift right
     * @return Reference to this object
     */
    endian_value &operator>>=(int shift)
    {
      *this = endian_value(to_native() >> shift);
      return *this;
    }
  };

  /**
   * @brief Specialized template for optimal case: native endianness with native type size
   *
   * This specialization provides optimal performance when the target endianness matches
   * the native endianness and the size matches the native type size. In this case,
   * no byte swapping is needed and operations can work directly on the native type.
   *
   * @tparam NativeType The underlying native integer type
   */
  template <typename NativeType>
  struct endian_value<std::endian::native, NativeType, sizeof(NativeType)>
  {
  private:
    /// Direct storage as native type for optimal performance
    NativeType data_;
    /// Size constant for consistency with general template
    static constexpr size_t Size = sizeof(NativeType);

  public:
    /// Type alias for the underlying native integer type
    using native_type = NativeType;

    /**
     * @brief Default constructor - initializes to zero
     */
    endian_value() = default;

    /**
     * @brief Constructor from native value
     * @param value The native value to store
     */
    constexpr endian_value(NativeType value) noexcept
        : data_(value)
    {
    }

    /**
     * @brief Get mutable pointer to the raw byte data
     * @return Pointer to the internal data as bytes
     */
    uint8_t *data() noexcept
    {
      return reinterpret_cast<uint8_t *>(&data_);
    }

    /**
     * @brief Get const pointer to the raw byte data
     * @return Const pointer to the internal data as bytes
     */
    const uint8_t *data() const noexcept
    {
      return reinterpret_cast<const uint8_t *>(&data_);
    }

    /**
     * @brief Get the size in bytes of the value
     * @return The size in bytes
     */
    constexpr size_t size() const noexcept
    {
      return Size;
    }

    /**
     * @brief Get the native value (no conversion needed)
     * @return The native value
     */
    constexpr NativeType to_native() const noexcept
    {
      return data_;
    }

    /**
     * @brief Implicit conversion to native type
     * @return The native value
     */
    constexpr operator NativeType() const noexcept
    {
      return data_;
    }

    /**
     * @brief Optimized equality comparison with another endian_value
     * @param other The other endian_value to compare with
     * @return true if values are equal, false otherwise
     */
    constexpr bool operator==(const endian_value &other) const noexcept
    {
      return data_ == other.data_;
    }

    /**
     * @brief Optimized inequality comparison with another endian_value
     * @param other The other endian_value to compare with
     * @return true if values are not equal, false otherwise
     */
    constexpr bool operator!=(const endian_value &other) const noexcept
    {
      return data_ != other.data_;
    }

    /**
     * @brief Optimized equality comparison with native type
     * @param value The native value to compare with
     * @return true if values are equal, false otherwise
     */
    constexpr bool operator==(NativeType value) const noexcept
    {
      return data_ == value;
    }

    /**
     * @brief Optimized inequality comparison with native type
     * @param value The native value to compare with
     * @return true if values are not equal, false otherwise
     */
    constexpr bool operator!=(NativeType value) const noexcept
    {
      return data_ != value;
    }

    /**
     * @brief Optimized bitwise OR with another endian_value
     * @param other The other endian_value to OR with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator|(const endian_value &other) const noexcept
    {
      return endian_value(data_ | other.data_);
    }

    /**
     * @brief Optimized bitwise OR with native type
     * @param value The native value to OR with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator|(NativeType value) const noexcept
    {
      return endian_value(data_ | value);
    }

    /**
     * @brief Optimized bitwise OR assignment with another endian_value
     * @param other The other endian_value to OR with
     * @return Reference to this object
     */
    endian_value &operator|=(const endian_value &other) noexcept
    {
      data_ |= other.data_;
      return *this;
    }

    /**
     * @brief Optimized bitwise OR assignment with native type
     * @param value The native value to OR with
     * @return Reference to this object
     */
    endian_value &operator|=(NativeType value) noexcept
    {
      data_ |= value;
      return *this;
    }

    /**
     * @brief Optimized bitwise AND with another endian_value
     * @param other The other endian_value to AND with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator&(const endian_value &other) const noexcept
    {
      return endian_value(data_ & other.data_);
    }

    /**
     * @brief Optimized bitwise AND with native type
     * @param value The native value to AND with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator&(NativeType value) const noexcept
    {
      return endian_value(data_ & value);
    }

    /**
     * @brief Optimized bitwise AND assignment with another endian_value
     * @param other The other endian_value to AND with
     * @return Reference to this object
     */
    endian_value &operator&=(const endian_value &other) noexcept
    {
      data_ &= other.data_;
      return *this;
    }

    /**
     * @brief Optimized bitwise AND assignment with native type
     * @param value The native value to AND with
     * @return Reference to this object
     */
    endian_value &operator&=(NativeType value) noexcept
    {
      data_ &= value;
      return *this;
    }

    /**
     * @brief Optimized bitwise XOR with another endian_value
     * @param other The other endian_value to XOR with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator^(const endian_value &other) const noexcept
    {
      return endian_value(data_ ^ other.data_);
    }

    /**
     * @brief Optimized bitwise XOR with native type
     * @param value The native value to XOR with
     * @return New value with the same endianess and size containing the result
     */
    endian_value operator^(NativeType value) const noexcept
    {
      return endian_value(data_ ^ value);
    }

    /**
     * @brief Optimized bitwise XOR assignment with another endian_value
     * @param other The other endian_value to XOR with
     * @return Reference to this object
     */
    endian_value &operator^=(const endian_value &other) noexcept
    {
      data_ ^= other.data_;
      return *this;
    }

    /**
     * @brief Optimized bitwise XOR assignment with native type
     * @param value The native value to XOR with
     * @return Reference to this object
     */
    endian_value &operator^=(NativeType value) noexcept
    {
      data_ ^= value;
      return *this;
    }

    /**
     * @brief Optimized bitwise NOT operation
     * @return New value with the same endianess and size with all bits inverted
     */
    endian_value operator~() const noexcept
    {
      return endian_value(~data_);
    }

    /**
     * @brief Optimized left shift operation
     * @param shift Number of bits to shift left
     * @return New value with the same endianess and size containing the shifted result
     */
    endian_value operator<<(int shift) const noexcept
    {
      return endian_value(data_ << shift);
    }

    /**
     * @brief Optimized left shift assignment
     * @param shift Number of bits to shift left
     * @return Reference to this object
     */
    endian_value &operator<<=(int shift) noexcept
    {
      data_ <<= shift;
      return *this;
    }

    /**
     * @brief Optimized right shift operation
     * @param shift Number of bits to shift right
     * @return New value with the same endianess and size containing the shifted result
     */
    endian_value operator>>(int shift) const noexcept
    {
      return endian_value(data_ >> shift);
    }

    /**
     * @brief Optimized right shift assignment
     * @param shift Number of bits to shift right
     * @return Reference to this object
     */
    endian_value &operator>>=(int shift) noexcept
    {
      data_ >>= shift;
      return *this;
    }
  };

  /**
   * @brief Free function for bitwise OR with native type on the left
   * @tparam Endian The endianness of the endian_value
   * @tparam NativeType The native integer type
   * @tparam Size The size in bytes
   * @param left The native value (left operand)
   * @param right The endian_value (right operand)
   * @return New endian_value containing the result
   */
  template <std::endian Endian, typename NativeType, size_t Size>
  endian_value<Endian, NativeType, Size> operator|(NativeType left, const endian_value<Endian, NativeType, Size> &right)
  {
    return endian_value<Endian, NativeType, Size>(left | right.to_native());
  }

  /**
   * @brief Free function for bitwise AND with native type on the left
   * @tparam Endian The endianness of the endian_value
   * @tparam NativeType The native integer type
   * @tparam Size The size in bytes
   * @param left The native value (left operand)
   * @param right The endian_value (right operand)
   * @return New endian_value containing the result
   */
  template <std::endian Endian, typename NativeType, size_t Size>
  endian_value<Endian, NativeType, Size> operator&(NativeType left, const endian_value<Endian, NativeType, Size> &right)
  {
    return endian_value<Endian, NativeType, Size>(left & right.to_native());
  }

  /**
   * @brief Free function for bitwise XOR with native type on the left
   * @tparam Endian The endianness of the endian_value
   * @tparam NativeType The native integer type
   * @tparam Size The size in bytes
   * @param left The native value (left operand)
   * @param right The endian_value (right operand)
   * @return New endian_value containing the result
   */
  template <std::endian Endian, typename NativeType, size_t Size>
  endian_value<Endian, NativeType, Size> operator^(NativeType left, const endian_value<Endian, NativeType, Size> &right)
  {
    return endian_value<Endian, NativeType, Size>(left ^ right.to_native());
  }

  /**
   * @brief Free function for equality comparison with native type on the left
   * @tparam Endian The endianness of the endian_value
   * @tparam NativeType The native integer type
   * @tparam Size The size in bytes
   * @param left The native value (left operand)
   * @param right The endian_value (right operand)
   * @return true if values are equal, false otherwise
   */
  template <std::endian Endian, typename NativeType, size_t Size>
  constexpr bool operator==(NativeType left, const endian_value<Endian, NativeType, Size> &right) noexcept
  {
    return left == right.to_native();
  }

  /**
   * @brief Free function for inequality comparison with native type on the left
   * @tparam Endian The endianness of the endian_value
   * @tparam NativeType The native integer type
   * @tparam Size The size in bytes
   * @param left The native value (left operand)
   * @param right The endian_value (right operand)
   * @return true if values are not equal, false otherwise
   */
  template <std::endian Endian, typename NativeType, size_t Size>
  constexpr bool operator!=(NativeType left, const endian_value<Endian, NativeType, Size> &right) noexcept
  {
    return left != right.to_native();
  }

  /**
   * @brief Type alias template for big-endian values
   * @tparam NativeType The native integer type that values can be converted to/from
   * @tparam Size The size in bytes (defaults to sizeof(NativeType))
   */
  template <typename NativeType, size_t Size = sizeof(NativeType)>
  using big_endian = endian_value<std::endian::big, NativeType, Size>;

  /**
   * @brief Type alias template for little-endian values
   * @tparam NativeType The native integer type that values can be converted to/from
   * @tparam Size The size in bytes (defaults to sizeof(NativeType))
   */
  template <typename NativeType, size_t Size = sizeof(NativeType)>
  using little_endian = endian_value<std::endian::little, NativeType, Size>;

  using i8 = endian_value<std::endian::native, int8_t, 1>;  ///< 8-bit signed integer
  using u8 = endian_value<std::endian::native, uint8_t, 1>; ///< 8-bit unsigned integer

  using i16be = big_endian<int16_t, 2>; ///< Big-endian 16-bit signed integer
  using i24be = big_endian<int32_t, 3>; ///< Big-endian 24-bit signed integer
  using i32be = big_endian<int32_t, 4>; ///< Big-endian 32-bit signed integer
  using i40be = big_endian<int64_t, 5>; ///< Big-endian 40-bit signed integer
  using i48be = big_endian<int64_t, 6>; ///< Big-endian 48-bit signed integer
  using i56be = big_endian<int64_t, 7>; ///< Big-endian 56-bit signed integer
  using i64be = big_endian<int64_t, 8>; ///< Big-endian 64-bit signed integer

  using i16le = little_endian<int16_t, 2>; ///< Little-endian 16-bit signed integer
  using i24le = little_endian<int32_t, 3>; ///< Little-endian 24-bit signed integer
  using i32le = little_endian<int32_t, 4>; ///< Little-endian 32-bit signed integer
  using i40le = little_endian<int64_t, 5>; ///< Little-endian 40-bit signed integer
  using i48le = little_endian<int64_t, 6>; ///< Little-endian 48-bit signed integer
  using i56le = little_endian<int64_t, 7>; ///< Little-endian 56-bit signed integer
  using i64le = little_endian<int64_t, 8>; ///< Little-endian 64-bit signed integer

  using u16be = big_endian<uint16_t, 2>; ///< Big-endian 16-bit unsigned integer
  using u24be = big_endian<uint32_t, 3>; ///< Big-endian 24-bit unsigned integer
  using u32be = big_endian<uint32_t, 4>; ///< Big-endian 32-bit unsigned integer
  using u40be = big_endian<uint64_t, 5>; ///< Big-endian 40-bit unsigned integer
  using u48be = big_endian<uint64_t, 6>; ///< Big-endian 48-bit unsigned integer
  using u56be = big_endian<uint64_t, 7>; ///< Big-endian 56-bit unsigned integer
  using u64be = big_endian<uint64_t, 8>; ///< Big-endian 64-bit unsigned integer

  using u16le = little_endian<uint16_t, 2>; ///< Little-endian 16-bit unsigned integer
  using u24le = little_endian<uint32_t, 3>; ///< Little-endian 24-bit unsigned integer
  using u32le = little_endian<uint32_t, 4>; ///< Little-endian 32-bit unsigned integer
  using u40le = little_endian<uint64_t, 5>; ///< Little-endian 40-bit unsigned integer
  using u48le = little_endian<uint64_t, 6>; ///< Little-endian 48-bit unsigned integer
  using u56le = little_endian<uint64_t, 7>; ///< Little-endian 56-bit unsigned integer
  using u64le = little_endian<uint64_t, 8>; ///< Little-endian 64-bit unsigned integer

} // namespace cjf

#endif // A5FD0473_0358_4683_A294_FBB4E5B3FA61
