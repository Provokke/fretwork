#include "fretwork/strings.hpp"
#include "harness.hpp"

#include <string>
#include <vector>

using namespace fretwork;

TEST(trim_removes_surrounding_whitespace)
{
    CHECK_EQ(trim("  hi \t\n"), "hi");
    CHECK_EQ(trim_left("  hi "), "hi ");
    CHECK_EQ(trim_right("  hi "), "  hi");
    CHECK_EQ(trim("   "), "");
    CHECK_EQ(trim(""), "");
}

TEST(split_on_delimiter)
{
    std::vector<std::string> expected = {"a", "b", "c"};
    CHECK(split("a,b,c", ",") == expected);
}

TEST(split_keeps_empty_fields)
{
    std::vector<std::string> expected = {"", "a", "", "b", ""};
    CHECK(split(",a,,b,", ",") == expected);
}

TEST(split_with_multichar_delimiter)
{
    std::vector<std::string> expected = {"one", "two", "three"};
    CHECK(split("one--two--three", "--") == expected);
}

TEST(split_without_delimiter_returns_whole_string)
{
    CHECK_EQ(split("abc", ",").size(), 1u);
    CHECK_EQ(split("abc", "")[0], "abc");
    CHECK_EQ(split("", ",").size(), 1u);
}

TEST(join_strings)
{
    std::vector<std::string> parts = {"x", "y", "z"};
    CHECK_EQ(join(parts, "-"), "x-y-z");
    CHECK_EQ(join(std::vector<std::string>{}, ","), "");
    CHECK_EQ(join(std::vector<std::string>{"solo"}, ","), "solo");
}

TEST(split_then_join_round_trips)
{
    std::string original = "alpha;beta;;gamma";
    CHECK_EQ(join(split(original, ";"), ";"), original);
}

TEST(replace_all_occurrences)
{
    CHECK_EQ(replace_all("a-b-c", "-", "+"), "a+b+c");
    CHECK_EQ(replace_all("aaa", "aa", "b"), "ba");
    CHECK_EQ(replace_all("abc", "x", "y"), "abc");
    CHECK_EQ(replace_all("abc", "", "y"), "abc");
    CHECK_EQ(replace_all("abc", "b", ""), "ac");
}

TEST(prefix_and_suffix_tests)
{
    CHECK(starts_with("fretwork", "fret"));
    CHECK(!starts_with("fret", "fretwork"));
    CHECK(ends_with("fretwork", "work"));
    CHECK(!ends_with("work", "fretwork"));
    CHECK(starts_with("abc", ""));
}

TEST(case_conversion)
{
    CHECK_EQ(to_upper("MiXed 123"), "MIXED 123");
    CHECK_EQ(to_lower("MiXed 123"), "mixed 123");
}

TEST(parse_integers)
{
    CHECK_EQ(*parse<int>("42"), 42);
    CHECK_EQ(*parse<int>("  -7  "), -7);
    CHECK_EQ(*parse<unsigned>("ff", 16), 255u);
    CHECK_EQ(*parse<long long>("9000000000"), 9000000000ll);
}

TEST(parse_reports_each_failure_kind)
{
    CHECK(parse<int>("").error() == ParseError::Empty);
    CHECK(parse<int>("   ").error() == ParseError::Empty);
    CHECK(parse<int>("12x").error() == ParseError::Invalid);
    CHECK(parse<int>("abc").error() == ParseError::Invalid);
    CHECK(parse<int>("99999999999").error() == ParseError::OutOfRange);
    CHECK(parse<unsigned>("-1").error() == ParseError::Invalid);
}

TEST(parse_doubles)
{
    CHECK_EQ(*parse<double>("2.5"), 2.5);
    CHECK_EQ(*parse<double>("-1e3"), -1000.0);
    CHECK(parse<double>("1.2.3").error() == ParseError::Invalid);
}

TEST(parse_composes_with_expected_combinators)
{
    auto doubled = parse<int>("21").map([](int v) { return v * 2; });
    CHECK_EQ(*doubled, 42);
    auto fallback = parse<int>("nope").value_or(-1);
    CHECK_EQ(fallback, -1);
    auto chained = parse<int>("8").and_then([](int v) { return parse<int>(std::to_string(v + 1)); });
    CHECK_EQ(*chained, 9);
}
