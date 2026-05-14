#include <doctest/doctest.h>

#include <string>

#include <cjf/params.h>
#include <cjf/params/mutable_param.h>

/* Helpers ══════════════════════════════════════════════════════════════════════════════════════ */

static int s_call_count = 0;
static cjf::param *s_last_param = nullptr;

static void count_callback(cjf::param &p, void *)
{
  ++s_call_count;
  s_last_param = &p;
}

/* mutable_param ════════════════════════════════════════════════════════════════════════════════ */

TEST_SUITE("mutable_param")
{
  TEST_CASE("default construction is null")
  {
    cjf::mutable_param<int> p;
    CHECK(!p.has_value());
  }

  TEST_CASE("typed construction")
  {
    cjf::mutable_param<int> p{42};
    REQUIRE(p.has_value());
    CHECK(p.get_as<int>() == 42);
  }

  TEST_CASE("construct from compatible type")
  {
    cjf::mutable_param<double> p{3}; // int -> double
    REQUIRE(p.has_value());
    CHECK(p.get_as<double>() == doctest::Approx(3.0));
  }

  TEST_CASE("construct from incompatible type results in null")
  {
    cjf::mutable_param<int> p{std::string_view("abc")};
    CHECK(!p.has_value());
  }

  TEST_CASE("construct with assignment from compatible type")
  {
    cjf::mutable_param<double> p = 3.14;
    REQUIRE(p.has_value());
    CHECK(p.get_as<double>() == doctest::Approx(3.14));
  }

  TEST_CASE("set with compatible type succeeds")
  {
    cjf::mutable_param<int> p;
    REQUIRE(p.set(99) == cjf::param_error::ok);
    CHECK(p.get_as<int>() == 99);
  }

  TEST_CASE("set with integer string parses to int")
  {
    cjf::mutable_param<int> p;
    REQUIRE(p.set(std::string_view("77")) == cjf::param_error::ok);
    CHECK(p.get_as<int>() == 77);
  }

  TEST_CASE("set with boolean string parses to bool")
  {
    cjf::mutable_param<bool> p;
    REQUIRE(p.set(std::string_view("true")) == cjf::param_error::ok);
    CHECK(p.get_as<bool>() == true);
  }

  TEST_CASE("set with double string parses to double")
  {
    cjf::mutable_param<double> p;
    REQUIRE(p.set(std::string_view("2.718")) == cjf::param_error::ok);
    CHECK(p.get_as<double>() == doctest::Approx(2.718));
  }

  TEST_CASE("set with invalid string returns invalid_cast")
  {
    cjf::mutable_param<int> p = 99;
    CHECK(p.set(std::string_view("abc")) == cjf::param_error::invalid_cast);
    CHECK(p.get_as<int>() == 99); // unchanged
  }

  TEST_CASE("set to null clears value")
  {
    cjf::mutable_param<int> p = 42;
    REQUIRE(p.set(cjf::param_null) == cjf::param_error::ok);
    CHECK(!p.has_value());
  }

  TEST_CASE("watch callback fires on set")
  {
    s_call_count = 0;
    cjf::mutable_param<int> p = 0;
    p.watch(&count_callback);
    p.set(1);
    CHECK(s_call_count == 1);
    CHECK(s_last_param == &p);
    p.unwatch(&count_callback);
  }

  TEST_CASE("unwatch stops callback firing")
  {
    s_call_count = 0;
    cjf::mutable_param<int> p{0};
    p.watch(&count_callback);
    p.unwatch(&count_callback);
    p.set(1);
    CHECK(s_call_count == 0);
  }

  TEST_CASE("multiple watchers all fire")
  {
    int a = 0, b = 0;
    auto cb_a = [](cjf::param &, void *ctx)
    { ++(*static_cast<int *>(ctx)); };
    auto cb_b = [](cjf::param &, void *ctx)
    { ++(*static_cast<int *>(ctx)); };

    cjf::mutable_param<int> p{0};
    p.watch(cb_a, &a);
    p.watch(cb_b, &b);
    p.set(1);
    CHECK(a == 1);
    CHECK(b == 1);
    p.unwatch(cb_a);
    p.unwatch(cb_b);
  }
}
