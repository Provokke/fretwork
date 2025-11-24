#include "fretwork/optional.hpp"
#include "harness.hpp"

#include <optional>
#include <string>

using namespace fretwork;

namespace {

std::optional<int> parse_digit(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    return std::nullopt;
}

}

TEST(optional_map_transforms_present_value)
{
    auto r = std::optional<int>(4) | map([](int v) { return v * 3; });
    CHECK(r.has_value());
    CHECK_EQ(*r, 12);
}

TEST(optional_map_keeps_empty_empty)
{
    auto r = std::optional<int>() | map([](int v) { return v * 3; });
    CHECK(!r.has_value());
}

TEST(optional_map_can_change_type)
{
    auto r = std::optional<int>(7) | map([](int v) { return std::to_string(v); });
    CHECK_EQ(*r, "7");
}

TEST(optional_and_then_flattens)
{
    auto r = std::optional<char>('8') | and_then(parse_digit);
    CHECK_EQ(*r, 8);
    auto none = std::optional<char>('x') | and_then(parse_digit);
    CHECK(!none.has_value());
}

TEST(optional_or_else_supplies_alternative)
{
    auto r = std::optional<int>() | or_else([] { return std::optional<int>(99); });
    CHECK_EQ(*r, 99);
    auto kept = std::optional<int>(1) | or_else([] { return std::optional<int>(99); });
    CHECK_EQ(*kept, 1);
}

TEST(optional_filter_drops_failing_values)
{
    auto even = [](int v) { return v % 2 == 0; };
    CHECK((std::optional<int>(4) | filter(even)).has_value());
    CHECK(!(std::optional<int>(5) | filter(even)).has_value());
    CHECK(!(std::optional<int>() | filter(even)).has_value());
}

TEST(optional_tap_observes_without_changing)
{
    int seen = 0;
    auto r = std::optional<int>(6) | tap([&](int v) { seen = v; });
    CHECK_EQ(seen, 6);
    CHECK_EQ(*r, 6);
    seen = 0;
    auto none = std::optional<int>() | tap([&](int v) { seen = v; });
    CHECK_EQ(seen, 0);
    CHECK(!none.has_value());
}

TEST(optional_value_or_ends_the_chain)
{
    int a = std::optional<int>(2) | value_or(0);
    int b = std::optional<int>() | value_or(-1);
    CHECK_EQ(a, 2);
    CHECK_EQ(b, -1);
}

TEST(optional_long_chain)
{
    std::string out = std::optional<char>('7') | and_then(parse_digit) | filter([](int d) { return d > 5; }) |
                      map([](int d) { return d * d; }) | map([](int d) { return std::to_string(d); }) |
                      value_or(std::string("none"));
    CHECK_EQ(out, "49");
    std::string miss = std::optional<char>('2') | and_then(parse_digit) | filter([](int d) { return d > 5; }) |
                       map([](int d) { return std::to_string(d); }) | value_or(std::string("none"));
    CHECK_EQ(miss, "none");
}

TEST(first_some_picks_first_present)
{
    auto r = first_some(std::optional<int>(), std::optional<int>(2), std::optional<int>(3));
    CHECK_EQ(*r, 2);
    auto none = first_some(std::optional<int>(), std::optional<int>());
    CHECK(!none.has_value());
    CHECK_EQ(*first_some(std::optional<int>(8)), 8);
}

TEST(some_if_guards_value)
{
    CHECK_EQ(*some_if(true, 5), 5);
    CHECK(!some_if(false, 5).has_value());
}

TEST(flatten_unwraps_one_level)
{
    std::optional<std::optional<int>> inner_some = std::optional<int>(3);
    std::optional<std::optional<int>> inner_none = std::optional<int>();
    std::optional<std::optional<int>> outer_none;
    CHECK_EQ(*flatten(inner_some), 3);
    CHECK(!flatten(inner_none).has_value());
    CHECK(!flatten(outer_none).has_value());
}

TEST(optional_chain_works_in_constexpr)
{
    constexpr auto r = std::optional<int>(2) | map([](int v) { return v + 1; });
    static_assert(r.has_value());
    static_assert(*r == 3);
    CHECK_EQ(*r, 3);
}

