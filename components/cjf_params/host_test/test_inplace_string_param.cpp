#include <doctest/doctest.h>

#include <string>

#include <cjf/params.h>
#include <cjf/params/inplace_string_param.h>

/* Helpers ══════════════════════════════════════════════════════════════════════════════════════ */

static int s_call_count = 0;
static cjf::param *s_last_param = nullptr;

static void count_callback(cjf::param &p, void *)
{
  ++s_call_count;
  s_last_param = &p;
}

/* inplace_string_param ═════════════════════════════════════════════════════════════════════════ */

TEST_SUITE("inplace_string_param")
{
  TEST_CASE("default construction is null")
  {
    cjf::inplace_string_param<16> p;
    CHECK(!p.has_value());
    CHECK(p.get_c_str() == nullptr);
  }

  TEST_CASE("construct from string_view")
  {
    cjf::inplace_string_param<16> p{std::string_view("hello")};
    REQUIRE(p.has_value());
    CHECK(std::get<std::string_view>(p.get()) == "hello");
  }

  TEST_CASE("construct from literal")
  {
    cjf::inplace_string_param<16> p{"world"};
    REQUIRE(p.has_value());
    CHECK(std::get<std::string_view>(p.get()) == "world");
  }

  TEST_CASE("get_c_str returns null-terminated string")
  {
    cjf::inplace_string_param<16> p{"test"};
    const char *cs = p.get_c_str();
    REQUIRE(cs != nullptr);
    CHECK(std::string_view(cs) == "test");
  }

  TEST_CASE("truncates to capacity")
  {
    cjf::inplace_string_param<4> p{std::string_view("hello!")};
    REQUIRE(p.has_value());
    CHECK(p.length() == 4);
    CHECK(std::get<std::string_view>(p.get()) == "hell");
  }

  TEST_CASE("set with numeric converts to string")
  {
    cjf::inplace_string_param<16> p;
    REQUIRE(p.set(42) == cjf::param_error::ok);
    CHECK(std::get<std::string_view>(p.get()) == "42");
  }

  TEST_CASE("set with bool converts to string")
  {
    cjf::inplace_string_param<8> p;
    REQUIRE(p.set(true) == cjf::param_error::ok);
    CHECK(std::get<std::string_view>(p.get()) == "true");
  }

  TEST_CASE("set to null clears value")
  {
    cjf::inplace_string_param<8> p{"hi"};
    REQUIRE(p.set(cjf::param_null) == cjf::param_error::ok);
    CHECK(!p.has_value());
    CHECK(p.get_c_str() == nullptr);
  }

  TEST_CASE("watch fires on set")
  {
    s_call_count = 0;
    cjf::inplace_string_param<16> p{"a"};
    p.watch(&count_callback);
    p.set(std::string_view("b"));
    CHECK(s_call_count == 1);
    p.unwatch(&count_callback);
  }

  TEST_CASE("capacity and size")
  {
    cjf::inplace_string_param<20> p{std::string_view("abc")};
    CHECK(p.capacity() == 20);
    CHECK(p.max_size() == 20);
    CHECK(p.size() == 3);
    CHECK(p.length() == 3);
  }
}
