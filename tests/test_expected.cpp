#include "fretwork/expected.hpp"
#include "harness.hpp"

#include <memory>
#include <string>

using fretwork::Expected;
using fretwork::unexpected;

namespace {

Expected<int, std::string> half(int n)
{
    if (n % 2 != 0) {
        return unexpected(std::string("odd"));
    }
    return n / 2;
}

}

TEST(expected_holds_value)
{
    Expected<int, std::string> e = 42;
    CHECK(e.has_value());
    CHECK(static_cast<bool>(e));
    CHECK_EQ(*e, 42);
    CHECK_EQ(e.value(), 42);
}

TEST(expected_holds_error)
{
    Expected<int, std::string> e = unexpected(std::string("boom"));
    CHECK(!e.has_value());
    CHECK_EQ(e.error(), "boom");
}

TEST(expected_value_throws_with_error_payload)
{
    Expected<int, std::string> e = unexpected(std::string("nope"));
    bool caught = false;
    try {
        (void)e.value();
    } catch (const fretwork::BadExpectedAccess<std::string>& ex) {
        caught = ex.error() == "nope";
    }
    CHECK(caught);
}

TEST(expected_value_or)
{
    Expected<int, std::string> good = 5;
    Expected<int, std::string> bad = unexpected(std::string("x"));
    CHECK_EQ(good.value_or(9), 5);
    CHECK_EQ(bad.value_or(9), 9);
}

TEST(expected_map_transforms_value)
{
    auto doubled = half(10).map([](int v) { return v * 2; });
    CHECK(doubled.has_value());
    CHECK_EQ(*doubled, 10);
    auto text = half(8).map([](int v) { return std::to_string(v); });
    CHECK_EQ(*text, "4");
}

TEST(expected_map_passes_error_through)
{
    auto r = half(3).map([](int v) { return v + 1; });
    CHECK(!r.has_value());
    CHECK_EQ(r.error(), "odd");
}

TEST(expected_map_error_rewrites_error)
{
    auto r = half(3).map_error([](const std::string& s) { return s.size(); });
    CHECK(!r.has_value());
    CHECK_EQ(r.error(), 3u);
    auto ok = half(4).map_error([](const std::string& s) { return s.size(); });
    CHECK_EQ(*ok, 2);
}

TEST(expected_and_then_chains_fallible_steps)
{
    auto r = half(8).and_then(half).and_then(half);
    CHECK(r.has_value());
    CHECK_EQ(*r, 1);
    auto stopped = half(12).and_then(half).and_then(half);
    CHECK(!stopped.has_value());
    CHECK_EQ(stopped.error(), "odd");
}

TEST(expected_or_else_recovers)
{
    auto r = half(7).or_else([](const std::string&) -> Expected<int, std::string> { return -1; });
    CHECK_EQ(*r, -1);
    auto untouched = half(4).or_else([](const std::string&) -> Expected<int, std::string> { return -1; });
    CHECK_EQ(*untouched, 2);
}

TEST(expected_equality)
{
    Expected<int, std::string> a = 1;
    Expected<int, std::string> b = 1;
    Expected<int, std::string> c = unexpected(std::string("e"));
    Expected<int, std::string> d = unexpected(std::string("e"));
    CHECK(a == b);
    CHECK(c == d);
    CHECK(!(a == c));
    CHECK(a == 1);
    CHECK(c == unexpected(std::string("e")));
}

TEST(expected_arrow_reaches_members)
{
    Expected<std::string, int> e = std::string("hello");
    CHECK_EQ(e->size(), 5u);
}

TEST(expected_supports_move_only_values)
{
    Expected<std::unique_ptr<int>, std::string> e = std::make_unique<int>(7);
    CHECK(e.has_value());
    auto taken = std::move(e).value();
    CHECK_EQ(*taken, 7);
}

TEST(expected_rvalue_map_moves_value)
{
    Expected<std::string, int> e = std::string("abc");
    auto sz = std::move(e).map([](std::string s) { return s.size(); });
    CHECK_EQ(*sz, 3u);
}

TEST(expected_to_optional)
{
    CHECK(half(4).to_optional().value() == 2);
    CHECK(!half(5).to_optional().has_value());
}

TEST(from_optional_attaches_error)
{
    auto some = fretwork::from_optional(std::optional<int>(3), std::string("missing"));
    auto none = fretwork::from_optional(std::optional<int>(), std::string("missing"));
    CHECK_EQ(*some, 3);
    CHECK_EQ(none.error(), "missing");
}

TEST(collect_gathers_values_or_first_error)
{
    std::vector<Expected<int, std::string>> good = {half(2), half(4), half(6)};
    auto all = fretwork::collect(good);
    CHECK(all.has_value());
    CHECK_EQ(all->size(), 3u);
    CHECK_EQ((*all)[2], 3);
    std::vector<Expected<int, std::string>> mixed = {half(2), half(3), half(5)};
    auto first_bad = fretwork::collect(mixed);
    CHECK(!first_bad.has_value());
    CHECK_EQ(first_bad.error(), "odd");
}

TEST(expected_is_usable_in_constexpr)
{
    constexpr Expected<int, int> e = 5;
    static_assert(e.has_value());
    static_assert(*e == 5);
    static_assert(e.value_or(1) == 5);
    CHECK(e.has_value());
}

