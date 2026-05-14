#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <array>
#include <cjf/params.h>
#include <cjf/params/const_param.h>
#include <cjf/params/enum_param.h>
#include <cjf/params/json_serialization.h>

#include <cJSON.h>
#include <map>
#include <string>

/* Helpers ══════════════════════════════════════════════════════════════════════════════════════ */

static int s_call_count = 0;
static cjf::param *s_last_param = nullptr;

static void count_callback(cjf::param &p, void *)
{
  ++s_call_count;
  s_last_param = &p;
}

// Simple enum used across enum_param tests
enum class colour
{
  red,
  green,
  blue
};

/* param_cast ═══════════════════════════════════════════════════════════════════════════════════ */

template <typename From, typename To>
void expect_param_cast(const From &from, const To &to)
{
  cjf::param_value v = from;
  auto result = cjf::param_cast<To>(v);
  REQUIRE(result.has_value());
  if constexpr (std::is_floating_point_v<To>)
  {
    CHECK(*result == doctest::Approx(to));
  }
  else
  {
    CHECK(*result == to);
  }
}

template <typename From, typename To>
void expect_cast_error(const From &from, cjf::param_error expected_err)
{
  cjf::param_value v = from;
  auto result = cjf::param_cast<To>(v);
  REQUIRE(!result.has_value());
  CHECK(result.error() == expected_err);
}

TEST_SUITE("param_cast")
{
  TEST_CASE("int -> int: same-type passthrough")
  {
    expect_param_cast<int, int>(42, 42);
  }

  TEST_CASE("int -> double: numeric widening")
  {
    expect_param_cast<int, double>(7, 7.0);
  }

  TEST_CASE("double -> int: numeric narrowing")
  {
    expect_param_cast<double, int>(3.0, 3);
  }

  TEST_CASE("int -> bool: zero is false")
  {
    expect_param_cast<int, bool>(0, false);
  }

  TEST_CASE("int -> bool: non-zero is true")
  {
    for (int val : {1, 42, -1})
      expect_param_cast<int, bool>(val, true);
  }

  TEST_CASE("string_view -> int: valid")
  {
    expect_param_cast<std::string_view, int>(std::string_view("123"), 123);
  }

  TEST_CASE("string_view -> int: invalid")
  {
    expect_cast_error<std::string_view, int>("abc", cjf::param_error::invalid_cast);
  }

  TEST_CASE("string_view -> bool: true variants")
  {
    for (const char *s : {"true", "True", "TRUE", "1"})
      expect_param_cast<std::string_view, bool>(s, true);
  }

  TEST_CASE("string_view -> bool: false variants")
  {
    for (const char *s : {"false", "False", "FALSE", "0", ""})
      expect_param_cast<std::string_view, bool>(s, false);
  }

  TEST_CASE("null -> any type is invalid_cast")
  {
    expect_cast_error<cjf::param_null_type, int>(cjf::param_null, cjf::param_error::invalid_cast);
    expect_cast_error<cjf::param_null_type, double>(cjf::param_null, cjf::param_error::invalid_cast);
    expect_cast_error<cjf::param_null_type, bool>(cjf::param_null, cjf::param_error::invalid_cast);
  }

  TEST_CASE("numeric -> string_view is invalid_cast")
  {
    expect_cast_error<int, std::string_view>(42, cjf::param_error::invalid_cast);
  }

  TEST_CASE("string_view -> double: valid")
  {
    expect_param_cast<std::string_view, double>(std::string_view("3.14"), 3.14);
  }

  TEST_CASE("unsigned int round-trip")
  {
    expect_param_cast<unsigned int, unsigned int>(99U, 99U);
  }
}

/* to_chars ═════════════════════════════════════════════════════════════════════════════════════ */

template <typename From, size_t BufferSize = 32>
void expect_to_chars(const From &from, const std::string &expected)
{
  char buf[BufferSize];
  cjf::param_value from_value = from;
  auto result = cjf::to_chars(buf, buf + sizeof(buf), from_value);
  REQUIRE(result.has_value());
  CHECK(*result == expected);
}

TEST_SUITE("to_chars")
{
  TEST_CASE("int")
  {
    expect_to_chars(42, "42");
  }

  TEST_CASE("negative int")
  {
    expect_to_chars(-10, "-10");
  }

  TEST_CASE("bool true")
  {
    expect_to_chars(true, "true");
  }

  TEST_CASE("bool false")
  {
    expect_to_chars(false, "false");
  }

  TEST_CASE("string_view passthrough")
  {
    expect_to_chars(std::string_view("hello"), "hello");
  }

  TEST_CASE("string_view buffer too small returns out_of_range")
  {
    char buf[4];
    auto result = cjf::to_chars(buf, buf + sizeof(buf), std::string_view("hello"));
    CHECK(!result.has_value());
    CHECK(result.error() == cjf::param_error::out_of_range);
  }

  TEST_CASE("null produces empty view")
  {
    char buf[8];
    auto result = cjf::to_chars(buf, buf + sizeof(buf), cjf::param_null);
    REQUIRE(result.has_value());
    CHECK(result->empty());
  }
  TEST_CASE("buffer too small returns out_of_range")
  {
    char buf[4];
    auto result = cjf::to_chars(buf, buf + sizeof(buf), 12345);
    CHECK(!result.has_value());
    CHECK(result.error() == cjf::param_error::out_of_range);
  }

  TEST_CASE("double")
  {
    expect_to_chars(3.14, "3.14");
  }
}

// // ─── const_param ──────────────────────────────────────────────────────────────

// TEST_SUITE("const_param")
// {
//   TEST_CASE("get returns stored value")
//   {
//     cjf::const_param p{42};
//     auto v = p.get();
//     REQUIRE(std::holds_alternative<int>(v));
//     CHECK(std::get<int>(v) == 42);
//   }

//   TEST_CASE("has_value true for non-null")
//   {
//     cjf::const_param p{std::string_view("hello")};
//     CHECK(p.has_value());
//   }

//   TEST_CASE("has_value false for null")
//   {
//     cjf::const_param p{cjf::param_null};
//     CHECK(!p.has_value());
//   }

//   TEST_CASE("set returns read_only")
//   {
//     cjf::const_param p{1};
//     CHECK(p.set(99) == cjf::param_error::read_only);
//   }

//   TEST_CASE("value unchanged after set")
//   {
//     cjf::const_param p{5};
//     p.set(99);
//     CHECK(std::get<int>(p.get()) == 5);
//   }

//   TEST_CASE("get_as round-trip int->double")
//   {
//     cjf::const_param p{7};
//     auto result = p.get_as<double>();
//     REQUIRE(result.has_value());
//     CHECK(*result == doctest::Approx(7.0));
//   }
// }

// // ─── enum_param ───────────────────────────────────────────────────────────────

// TEST_SUITE("enum_param")
// {
//   TEST_CASE("construct with enum value")
//   {
//     cjf::enum_param<colour> p{colour::red};
//     REQUIRE(p.has_value());
//     CHECK(p.get_enum() == colour::red);
//   }

//   TEST_CASE("get returns enum name as string_view")
//   {
//     cjf::enum_param<colour> p{colour::green};
//     auto v = p.get();
//     REQUIRE(std::holds_alternative<std::string_view>(v));
//     CHECK(std::get<std::string_view>(v) == "green");
//   }

//   TEST_CASE("default construction is null")
//   {
//     cjf::enum_param<colour> p;
//     CHECK(!p.has_value());
//   }

//   TEST_CASE("set from string name")
//   {
//     cjf::enum_param<colour> p;
//     CHECK(p.set(std::string_view("blue")) == cjf::param_error::ok);
//     CHECK(p.get_enum() == colour::blue);
//   }

//   TEST_CASE("set from underlying integral value")
//   {
//     cjf::enum_param<colour> p;
//     CHECK(p.set(0) == cjf::param_error::ok); // red == 0
//     CHECK(p.get_enum() == colour::red);
//   }

//   TEST_CASE("set from invalid string returns invalid_cast")
//   {
//     cjf::enum_param<colour> p{colour::red};
//     CHECK(p.set(std::string_view("purple")) == cjf::param_error::invalid_cast);
//     CHECK(p.get_enum() == colour::red); // unchanged
//   }

//   TEST_CASE("set to null clears value")
//   {
//     cjf::enum_param<colour> p{colour::blue};
//     CHECK(p.set(cjf::param_null) == cjf::param_error::ok);
//     CHECK(!p.has_value());
//   }

//   TEST_CASE("set_enum fires watch callback")
//   {
//     s_call_count = 0;
//     cjf::enum_param<colour> p{colour::red};
//     p.watch(&count_callback);
//     p.set_enum(colour::green);
//     CHECK(s_call_count == 1);
//     p.unwatch(&count_callback);
//   }

//   TEST_CASE("operator== with enum value")
//   {
//     cjf::enum_param<colour> p{colour::blue};
//     CHECK(p == colour::blue);
//     CHECK(!(p == colour::red));
//   }

//   TEST_CASE("watch fires on set")
//   {
//     s_call_count = 0;
//     cjf::enum_param<colour> p{colour::red};
//     p.watch(&count_callback);
//     p.set(std::string_view("green"));
//     CHECK(s_call_count == 1);
//     p.unwatch(&count_callback);
//   }
// }

// // ─── json_serialization ───────────────────────────────────────────────────────

// TEST_SUITE("json_serialization")
// {
//   // Helper: parse a JSON string and run from_json
//   static cjf::param_value parse_value(const char *json_str)
//   {
//     cJSON *j = cJSON_Parse(json_str);
//     REQUIRE(j != nullptr);
//     auto result = cjf::from_json(j);
//     cJSON_Delete(j);
//     REQUIRE(result.has_value());
//     return *result;
//   }

//   TEST_CASE("to_json: int param_value")
//   {
//     char buf[64];
//     auto result = cjf::to_json(buf, buf + sizeof(buf), cjf::param_value{42});
//     REQUIRE(result.has_value());
//     CHECK(*result == "42");
//   }

//   TEST_CASE("to_json: bool param_value true")
//   {
//     char buf[16];
//     auto result = cjf::to_json(buf, buf + sizeof(buf), cjf::param_value{true});
//     REQUIRE(result.has_value());
//     CHECK(*result == "true");
//   }

//   TEST_CASE("to_json: bool param_value false")
//   {
//     char buf[16];
//     auto result = cjf::to_json(buf, buf + sizeof(buf), cjf::param_value{false});
//     REQUIRE(result.has_value());
//     CHECK(*result == "false");
//   }

//   TEST_CASE("to_json: string_view param_value")
//   {
//     char buf[64];
//     auto result = cjf::to_json(buf, buf + sizeof(buf), cjf::param_value{std::string_view("hello")});
//     REQUIRE(result.has_value());
//     CHECK(*result == "\"hello\"");
//   }

//   TEST_CASE("to_json: null param_value")
//   {
//     char buf[16];
//     auto result = cjf::to_json(buf, buf + sizeof(buf), cjf::param_value{cjf::param_null});
//     REQUIRE(result.has_value());
//     CHECK(*result == "null");
//   }

//   TEST_CASE("to_json: double param_value")
//   {
//     char buf[64];
//     auto result = cjf::to_json(buf, buf + sizeof(buf), cjf::param_value{1.5});
//     REQUIRE(result.has_value());
//     // Parse back and check the numeric value
//     cJSON *j = cJSON_Parse(std::string(*result).c_str());
//     REQUIRE(j != nullptr);
//     CHECK(cJSON_IsNumber(j));
//     CHECK(j->valuedouble == doctest::Approx(1.5));
//     cJSON_Delete(j);
//   }

//   TEST_CASE("to_json via param overload")
//   {
//     char buf[64];
//     cjf::const_param p{99};
//     auto result = cjf::to_json(buf, buf + sizeof(buf), static_cast<const cjf::param &>(p));
//     REQUIRE(result.has_value());
//     CHECK(*result == "99");
//   }

//   TEST_CASE("from_json: integer")
//   {
//     auto v = parse_value("42");
//     // cJSON parses numbers as double; after from_json it may be double or int
//     // Check by casting to double either way
//     auto d = cjf::param_cast<double>(v);
//     REQUIRE(d.has_value());
//     CHECK(*d == doctest::Approx(42.0));
//   }

//   TEST_CASE("from_json: string")
//   {
//     auto v = parse_value("\"world\"");
//     REQUIRE(std::holds_alternative<std::string_view>(v));
//     CHECK(std::get<std::string_view>(v) == "world");
//   }

//   TEST_CASE("from_json: bool true")
//   {
//     auto v = parse_value("true");
//     REQUIRE(std::holds_alternative<bool>(v));
//     CHECK(std::get<bool>(v) == true);
//   }

//   TEST_CASE("from_json: bool false")
//   {
//     auto v = parse_value("false");
//     REQUIRE(std::holds_alternative<bool>(v));
//     CHECK(std::get<bool>(v) == false);
//   }

//   TEST_CASE("from_json: null")
//   {
//     auto v = parse_value("null");
//     CHECK(std::holds_alternative<cjf::param_null_type>(v));
//   }

//   TEST_CASE("from_json sets mutable_param via param overload")
//   {
//     cjf::mutable_param<int> p{0};
//     cJSON *j = cJSON_Parse("55");
//     REQUIRE(j != nullptr);
//     auto err = cjf::from_json(static_cast<cjf::param &>(p), j);
//     cJSON_Delete(j);
//     CHECK(err == ESP_OK);
//     auto v = p.get_as<int>();
//     REQUIRE(v.has_value());
//     CHECK(*v == 55);
//   }

//   TEST_CASE("to_json and from_json map round-trip")
//   {
//     cjf::mutable_param<int> count{10};
//     cjf::inplace_string_param<32> name{"test"};
//     std::map<const char *, cjf::param *> params{
//         {"count", &count},
//         {"name", &name},
//     };

//     char buf[256];
//     auto json_result = cjf::to_json(buf, buf + sizeof(buf), params);
//     REQUIRE(json_result.has_value());

//     // Parse back
//     cjf::mutable_param<int> count2;
//     cjf::inplace_string_param<32> name2;
//     std::map<const char *, cjf::param *> params2{
//         {"count", &count2},
//         {"name", &name2},
//     };

//     cJSON *j = cJSON_Parse(std::string(*json_result).c_str());
//     REQUIRE(j != nullptr);
//     auto err = cjf::from_json(params2, j);
//     cJSON_Delete(j);
//     CHECK(err == ESP_OK);

//     auto c = count2.get_as<int>();
//     REQUIRE(c.has_value());
//     CHECK(*c == 10);

//     auto n = name2.get_as<std::string_view>();
//     REQUIRE(n.has_value());
//     CHECK(*n == "test");
//   }
// }
