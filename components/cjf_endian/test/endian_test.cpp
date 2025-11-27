#include <cjf/endian.h>
#include <unity.h>

template <class T>
void endian_test_case(const typename T::native_type native_value, const uint8_t* expected_data, const size_t expected_size)
{
  T value(native_value);
  TEST_ASSERT_EQUAL(expected_size, value.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_data, value.data(), expected_size);
  TEST_ASSERT_EQUAL(native_value, value.to_native());
  TEST_ASSERT_EQUAL(native_value, static_cast<T::native_type>(value));
}


TEST_CASE("i16be", "[endian]")
{
  int16_t native_value = -292;
  uint8_t expected_data[] = {0xFE, 0xDC};
  endian_test_case<cjf::i16be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("i16le", "[endian]")
{
  int16_t native_value = -292;
  uint8_t expected_data[] = {0xDC, 0xFE};
  endian_test_case<cjf::i16le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("u16be", "[endian]")
{
  uint16_t native_value = 0x1234;
  uint8_t expected_data[] = {0x12, 0x34};
  endian_test_case<cjf::u16be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("u16le", "[endian]")
{
  uint16_t native_value = 0xABCD;
  uint8_t expected_data[] = {0xCD, 0xAB};
  endian_test_case<cjf::u16le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("i24be", "[endian]")
{
  int32_t native_value = -74566;
  uint8_t expected_data[] = {0xFE, 0xDC, 0xBA};
  endian_test_case<cjf::i24be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("i24le", "[endian]")
{
  int32_t native_value = -74566;
  uint8_t expected_data[] = {0xBA, 0xDC, 0xFE};
  endian_test_case<cjf::i24le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("u24be", "[endian]")
{
  uint32_t native_value = 0x00123456;
  uint8_t expected_data[] = {0x12, 0x34, 0x56};
  endian_test_case<cjf::u24be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("u24le", "[endian]")
{
  uint32_t native_value = 0x00ABCDEF;
  uint8_t expected_data[] = {0xEF, 0xCD, 0xAB};
  endian_test_case<cjf::u24le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("i32be", "[endian]")
{
  int32_t native_value = -19088744;
  uint8_t expected_data[] = {0xFE, 0xDC, 0xBA, 0x98};
  endian_test_case<cjf::i32be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("i32le", "[endian]")
{
  int32_t native_value = -19088744;
  uint8_t expected_data[] = {0x98, 0xBA, 0xDC, 0xFE};
  endian_test_case<cjf::i32le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("u32be", "[endian]")
{
  uint32_t native_value = 0x12345678;
  uint8_t expected_data[] = {0x12, 0x34, 0x56, 0x78};
  endian_test_case<cjf::u32be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("u32le", "[endian]")
{
  uint32_t native_value = 0xDEADBEEF;
  uint8_t expected_data[] = {0xEF, 0xBE, 0xAD, 0xDE};
  endian_test_case<cjf::u32le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("i40be", "[endian]")
{
  int64_t native_value = -4886718346;
  uint8_t expected_data[] = {0xFE, 0xDC, 0xBA, 0x98, 0x76};
  endian_test_case<cjf::i40be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("i40le", "[endian]")
{
  int64_t native_value = -4886718346;
  uint8_t expected_data[] = {0x76, 0x98, 0xBA, 0xDC, 0xFE};
  endian_test_case<cjf::i40le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("u40be", "[endian]")
{
  uint64_t native_value = 0x0000001122334455;
  uint8_t expected_data[] = {0x11, 0x22, 0x33, 0x44, 0x55};
  endian_test_case<cjf::u40be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("u40le", "[endian]")
{
  uint64_t native_value = 0x000000AABBCCDDEE;
  uint8_t expected_data[] = {0xEE, 0xDD, 0xCC, 0xBB, 0xAA};
  endian_test_case<cjf::u40le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("i48be", "[endian]")
{
  int64_t native_value = -1250999896492;
  uint8_t expected_data[] = {0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54};
  endian_test_case<cjf::i48be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("i48le", "[endian]")
{
  int64_t native_value = -1250999896492;
  uint8_t expected_data[] = {0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE};
  endian_test_case<cjf::i48le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("u48be", "[endian]")
{
  uint64_t native_value = 0x0000112233445566;
  uint8_t expected_data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  endian_test_case<cjf::u48be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("u48le", "[endian]")
{
  uint64_t native_value = 0x0000AABBCCDDEEFF;
  uint8_t expected_data[] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA};
  endian_test_case<cjf::u48le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("i56be", "[endian]")
{
  int64_t native_value = -320255973501902;
  uint8_t expected_data[] = {0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32};
  endian_test_case<cjf::i56be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("i56le", "[endian]")
{
  int64_t native_value = -320255973501902;
  uint8_t expected_data[] = {0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE};
  endian_test_case<cjf::i56le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("u56be", "[endian]")
{
  uint64_t native_value = 0x0011223344556677;
  uint8_t expected_data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  endian_test_case<cjf::u56be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("u56le", "[endian]")
{
  uint64_t native_value = 0x00AAABBBCCCDDDEE;
  uint8_t expected_data[] = {0xEE, 0xDD, 0xCD, 0xCC, 0xBB, 0xAB, 0xAA};
  endian_test_case<cjf::u56le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("i64be", "[endian]")
{
  int64_t native_value = -81985529216486896;
  uint8_t expected_data[] = {0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};
  endian_test_case<cjf::i64be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("i64le", "[endian]")
{
  int64_t native_value = -81985529216486896;
  uint8_t expected_data[] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE};
  endian_test_case<cjf::i64le>(native_value, expected_data, sizeof(expected_data));
}

TEST_CASE("u64be", "[endian]")
{
  uint64_t native_value = 0x1122334455667788;
  uint8_t expected_data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
  endian_test_case<cjf::u64be>(native_value, expected_data, sizeof(expected_data));

}

TEST_CASE("u64le", "[endian]")
{
  uint64_t native_value = 0xAAABBBCCCDDDEEEF;
  uint8_t expected_data[] = {0xEF, 0xEE, 0xDD, 0xCD, 0xCC, 0xBB, 0xAB, 0xAA};
  endian_test_case<cjf::u64le>(native_value, expected_data, sizeof(expected_data));
}

// Comparison operator tests

TEST_CASE("Equality comparison with another endian_value (endian_value == endian_value)", "[endian]")
{
  cjf::u24be value1(0x123456);
  cjf::u24be value2(0x123456);
  cjf::u24be value3(0x567890);

  TEST_ASSERT_TRUE(value1 == value2);
  TEST_ASSERT_FALSE(value1 == value3);
}

TEST_CASE("Equality comparison with native type (endian_value == native)", "[endian]")
{
  cjf::u24be value(0x123456);

  TEST_ASSERT_TRUE(value == 0x123456);
  TEST_ASSERT_FALSE(value == 0x567890);
}

TEST_CASE("Equality comparison with native type (native == endian_value)", "[endian]")
{
  cjf::u24be value(0x123456);

  TEST_ASSERT_TRUE(0x123456 == value);
  TEST_ASSERT_FALSE(0x567890 == value);
}

TEST_CASE("Inequality comparison with another endian_value (endian_value != endian_value)", "[endian]")
{
  cjf::u24be value1(0x123456);
  cjf::u24be value2(0x123456);
  cjf::u24be value3(0x567890);

  TEST_ASSERT_FALSE(value1 != value2);
  TEST_ASSERT_TRUE(value1 != value3);
}

TEST_CASE("Inequality comparison with native type (endian_value != native)", "[endian]")
{
  cjf::u24be value(0x123456);

  TEST_ASSERT_FALSE(value != 0x123456);
  TEST_ASSERT_TRUE(value != 0x567890);
}

TEST_CASE("Inequality comparison with native type (native != endian_value)", "[endian]")
{
  cjf::u24be value(0x123456);

  TEST_ASSERT_FALSE(0x123456 != value);
  TEST_ASSERT_TRUE(0x567890 != value);
}

TEST_CASE("Comparison operators (native type specialization)", "[endian]")
{
  // Test the specialized template for native endianness
  cjf::i8 value1(42);
  cjf::i8 value2(42);
  cjf::i8 value3(24);

  TEST_ASSERT_TRUE(value1 == value2);
  TEST_ASSERT_FALSE(value1 == value3);
  TEST_ASSERT_FALSE(value1 != value2);
  TEST_ASSERT_TRUE(value1 != value3);
  TEST_ASSERT_TRUE(value1 == 42);
  TEST_ASSERT_TRUE(42 == value1);
}

// Bitwise OR operator tests

TEST_CASE("Bitwise OR with another endian_value (endian_value | endian_value)", "[endian]")
{
  cjf::u24be value1(0x123456);
  cjf::u24be value2(0x560000);

  auto result = value1 | value2;
  TEST_ASSERT_EQUAL(0x563456, result.to_native());
}

TEST_CASE("Bitwise OR with another endian_value (native type specialization)", "[endian]")
{
  cjf::u16le value1(0x1234);
  cjf::u16le value2(0x5600);

  auto result = value1 | value2;
  TEST_ASSERT_EQUAL(0x5634, result.to_native());
}

TEST_CASE("Bitwise OR with native type (endian_value | native)", "[endian]")
{
  cjf::u24be value(0x123456);

  auto result = value | 0x00FF00;
  TEST_ASSERT_EQUAL(0x12FF56, result.to_native());
}

TEST_CASE("Bitwise OR with native type (native | endian_value)", "[endian]")
{
  cjf::u24be value(0x123456);

  uint32_t result = 0xF00000 | value;
  TEST_ASSERT_EQUAL(0xF23456, result);
}

TEST_CASE("Bitwise OR assignment with another endian_value (endian_value |= endian_value)", "[endian]")
{
  cjf::u24be value1(0x123456);
  cjf::u24be value2(0x560000);

  value1 |= value2;
  TEST_ASSERT_EQUAL(0x563456, value1.to_native());
}

TEST_CASE("Bitwise OR assignment with another endian_value (native type specialization)", "[endian]")
{
  cjf::u16le value1(0x1234);
  cjf::u16le value2(0x5600);

  value1 |= value2;
  TEST_ASSERT_EQUAL(0x5634, value1.to_native());
}

TEST_CASE("Bitwise OR assignment with native type", "[endian]")
{
  cjf::u24be value(0x123456);

  value |= 0x00FF00;
  TEST_ASSERT_EQUAL(0x12FF56, value.to_native());
}

// Bitwise AND operator tests

TEST_CASE("Bitwise AND with another endian_value (endian_value & endian_value)", "[endian]")
{
  cjf::u24be value1(0xF0F0F0);
  cjf::u24be value2(0xFF0000);

  auto result = value1 & value2;
  TEST_ASSERT_EQUAL(0xF00000, result.to_native());
}

TEST_CASE("Bitwise AND with another endian_value (native type specialization)", "[endian]")
{
  cjf::u16le value1(0xF0F0);
  cjf::u16le value2(0xFF00);

  auto result = value1 & value2;
  TEST_ASSERT_EQUAL(0xF000, result.to_native());
}

TEST_CASE("Bitwise AND with native type (endian_value & native)", "[endian]")
{
  cjf::u24be value(0xF0F0F0);

  auto result = value & 0x0F0F0F;
  TEST_ASSERT_EQUAL(0x000000, result.to_native());
}

TEST_CASE("Bitwise AND with native type (native & endian_value)", "[endian]")
{
  cjf::u24be value(0xF0F0F0);

  uint32_t result = 0xFFFFFF & value;
  TEST_ASSERT_EQUAL(0xF0F0F0, result);
}

TEST_CASE("Bitwise AND assignment with another endian_value (endian_value &= endian_value)", "[endian]")
{
  cjf::u24be value1(0xF0F0F0);
  cjf::u24be value2(0xFF00FF);

  value1 &= value2;
  TEST_ASSERT_EQUAL(0xF000F0, value1.to_native());
}

TEST_CASE("Bitwise AND assignment with another endian_value (native type specialization)", "[endian]")
{
  cjf::u16le value1(0xF0F0);
  cjf::u16le value2(0xFF00);

  value1 &= value2;
  TEST_ASSERT_EQUAL(0xF000, value1.to_native());
}

TEST_CASE("Bitwise AND assignment with native type", "[endian]")
{
  cjf::u24be value(0xF0F0F0);

  value &= 0x0F0F0F;
  TEST_ASSERT_EQUAL(0x000000, value.to_native());
}

// Bitwise XOR operator tests

TEST_CASE("Bitwise XOR with another endian_value (endian_value ^ endian_value)", "[endian]")
{
  cjf::u24be value1(0xF0F0F0);
  cjf::u24be value2(0xFF00FF);

  auto result = value1 ^ value2;
  TEST_ASSERT_EQUAL(0x0FF00F, result.to_native());
}

TEST_CASE("Bitwise XOR with another endian_value (native type specialization)", "[endian]")
{
  cjf::u16le value1(0xF0F0);
  cjf::u16le value2(0xFF00);

  auto result = value1 ^ value2;
  TEST_ASSERT_EQUAL(0x0FF0, result.to_native());
}

TEST_CASE("Bitwise XOR with native type (endian_value ^ native)", "[endian]")
{
  cjf::u24be value(0xF0F0F0);

  auto result = value ^ 0xFFFFFF;
  TEST_ASSERT_EQUAL(0x0F0F0F, result.to_native());
}

TEST_CASE("Bitwise XOR with native type (native ^ endian_value)", "[endian]")
{
  cjf::u24be value(0xF0F0F0);

  uint32_t result = 0xAAAAAA ^ value;
  TEST_ASSERT_EQUAL(0x5A5A5A, result);
}

TEST_CASE("Bitwise XOR assignment with another endian_value (endian_value ^= endian_value)", "[endian]")
{
  cjf::u24be value1(0xF0F0F0);
  cjf::u24be value2(0xFF00FF);

  value1 ^= value2;
  TEST_ASSERT_EQUAL(0x0FF00F, value1.to_native());
}

TEST_CASE("Bitwise XOR assignment with another endian_value (native type specialization)", "[endian]")
{
  cjf::u16le value1(0xF0F0);
  cjf::u16le value2(0xFF00);

  value1 ^= value2;
  TEST_ASSERT_EQUAL(0x0FF0, value1.to_native());
}

TEST_CASE("Bitwise XOR assignment with native type", "[endian]")
{
  cjf::u24be value(0xF0F0F0);

  value ^= 0xFFFFFF;
  TEST_ASSERT_EQUAL(0x0F0F0F, value.to_native());
}

// Bitwise NOT operator tests

TEST_CASE("Bitwise NOT operator", "[endian]")
{
  cjf::u24be value(0xF0F0F0);
  auto result = ~value;
  TEST_ASSERT_EQUAL(0x0F0F0F, result.to_native());
}

TEST_CASE("Bitwise NOT operator (native type specialization)", "[endian]")
{
  cjf::u16le value(0xF0F0);
  auto result = ~value;
  TEST_ASSERT_EQUAL(0x0F0F, result.to_native());
}

TEST_CASE("Bitwise NOT operator with different endianness", "[endian]")
{
  cjf::u16le value(0xF0F0);
  auto result = ~value;
  TEST_ASSERT_EQUAL(0x0F0F, result.to_native());
}

// Shift operator tests

TEST_CASE("Left shift operator", "[endian]")
{
  cjf::u24be value(0x123456);

  auto result = value << 4;
  TEST_ASSERT_EQUAL(0x234560, result.to_native());
}

TEST_CASE("Left shift operator (native type specialization)", "[endian]")
{
  cjf::u16le value(0x1234);

  auto result = value << 4;
  TEST_ASSERT_EQUAL(0x2340, result.to_native());
}

TEST_CASE("Left shift assignment operator", "[endian]")
{
  cjf::u24be value(0x123456);

  value <<= 4;
  TEST_ASSERT_EQUAL(0x234560, value.to_native());
}

TEST_CASE("Left shift assignment operator (native type specialization)", "[endian]")
{
  cjf::u16le value(0x1234);

  value <<= 4;
  TEST_ASSERT_EQUAL(0x2340, value.to_native());
}

TEST_CASE("Right shift operator", "[endian]")
{
  cjf::u24be value(0x123456);

  auto result = value >> 4;
  TEST_ASSERT_EQUAL(0x012345, result.to_native());
}

TEST_CASE("Right shift operator (native type specialization)", "[endian]")
{
  cjf::u16le value(0x1234);

  auto result = value >> 4;
  TEST_ASSERT_EQUAL(0x0123, result.to_native());
}

TEST_CASE("Right shift assignment operator", "[endian]")
{
  cjf::u24be value(0x123456);

  value >>= 4;
  TEST_ASSERT_EQUAL(0x012345, value.to_native());
}

TEST_CASE("Right shift assignment operator (native type specialization)", "[endian]")
{
  cjf::u16le value(0x1234);

  value >>= 4;
  TEST_ASSERT_EQUAL(0x0123, value.to_native());
}

TEST_CASE("Right shift with signed value preserves sign", "[endian]")
{
  cjf::i16be signed_value(-1); // 0xFFFF
  auto signed_result = signed_value >> 4;
  TEST_ASSERT_EQUAL(-1, signed_result.to_native()); // Should preserve sign
}

TEST_CASE("Shift operations with 40-bit value", "[endian]")
{
  cjf::u40be value40(0x123456789A);
  auto shift40 = value40 >> 4;
  TEST_ASSERT_EQUAL_HEX64(0x0123456789, shift40.to_native());
}

TEST_CASE("Operations between different endianness", "[endian]")
{
  cjf::u16be be_value(0x1234);
  cjf::u16le le_value(0x1234);

  // Both should have the same logical value
  TEST_ASSERT_EQUAL(0x1234, be_value.to_native());
  TEST_ASSERT_EQUAL(0x1234, le_value.to_native());

  // Bitwise operations should work on the logical values
  auto result = be_value | le_value;
  TEST_ASSERT_EQUAL(0x1234, result.to_native());
}

// Complex usage patterns

TEST_CASE("Register bit manipulation pattern", "[endian]")
{
  cjf::u16be register_value(0b1010101010101010);
  const uint16_t ENABLE_MASK = 0b0000000000000001;

  // Set enable bit
  register_value |= ENABLE_MASK;
  TEST_ASSERT_EQUAL(0b1010101010101011, register_value.to_native());

  // Clear enable bit
  register_value &= ~ENABLE_MASK;
  TEST_ASSERT_EQUAL(0b1010101010101010, register_value.to_native());
}

TEST_CASE("Register read-modify-write pattern", "[endian]")
{
  cjf::u16be register_value(0b1010101010101010);
  const uint16_t CONFIG_MASK = 0b0000000011110000;
  const uint16_t NEW_CONFIG = 0b0000000001010000;

  // Clear and set config bits
  register_value = (register_value & ~CONFIG_MASK) | NEW_CONFIG;
  TEST_ASSERT_EQUAL(0b1010101001011010, register_value.to_native());
}
