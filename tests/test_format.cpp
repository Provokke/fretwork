#include "fretwork/format.hpp"
#include "harness.hpp"

#include <limits>
#include <ostream>
#include <string>

using fretwork::format;
using fretwork::FormatErrorKind;
using fretwork::try_format;

namespace {

struct Point {
    int x;
    int y;
};

std::ostream& operator<<(std::ostream& os, const Point& p)
{
    return os << "(" << p.x << ", " << p.y << ")";
}

enum class Colour { Red = 1, Green = 2 };

}

TEST(format_plain_text_passes_through)
{
    CHECK_EQ(format("hello"), "hello");
    CHECK_EQ(format(""), "");
}

TEST(format_automatic_indexing)
{
    CHECK_EQ(format("{} + {} = {}", 1, 2, 3), "1 + 2 = 3");
}

TEST(format_explicit_indexing_reorders_and_repeats)
{
    CHECK_EQ(format("{1} {0} {1}", "a", "b"), "b a b");
}

TEST(format_escaped_braces)
{
    CHECK_EQ(format("{{}} {}", 5), "{} 5");
    CHECK_EQ(format("a}}b"), "a}b");
}

TEST(format_strings_and_chars)
{
    std::string owned = "owned";
    CHECK_EQ(format("{} {} {}", "literal", owned, 'c'), "literal owned c");
    CHECK_EQ(format("{}", std::string_view("view")), "view");
}

TEST(format_booleans)
{
    CHECK_EQ(format("{} {}", true, false), "true false");
    CHECK_EQ(format("{:d}", true), "1");
}

TEST(format_integer_types)
{
    CHECK_EQ(format("{}", -17), "-17");
    CHECK_EQ(format("{}", 18446744073709551615ull), "18446744073709551615");
    CHECK_EQ(format("{}", std::numeric_limits<long long>::min()), "-9223372036854775808");
    CHECK_EQ(format("{}", static_cast<short>(-3)), "-3");
}

TEST(format_radix_specifiers)
{
    CHECK_EQ(format("{:x}", 255), "ff");
    CHECK_EQ(format("{:X}", 255), "FF");
    CHECK_EQ(format("{:b}", 5), "101");
    CHECK_EQ(format("{:o}", 64), "100");
    CHECK_EQ(format("{:#x}", 255), "0xff");
    CHECK_EQ(format("{:#X}", 255), "0XFF");
    CHECK_EQ(format("{:#b}", 5), "0b101");
}

TEST(format_width_and_alignment)
{
    CHECK_EQ(format("[{:5}]", 42), "[   42]");
    CHECK_EQ(format("[{:5}]", "ab"), "[ab   ]");
    CHECK_EQ(format("[{:<5}]", 42), "[42   ]");
    CHECK_EQ(format("[{:>5}]", "ab"), "[   ab]");
    CHECK_EQ(format("[{:^6}]", "ab"), "[  ab  ]");
    CHECK_EQ(format("[{:^5}]", "ab"), "[ ab  ]");
}

TEST(format_custom_fill)
{
    CHECK_EQ(format("[{:*^7}]", "hi"), "[**hi***]");
    CHECK_EQ(format("[{:-<4}]", 1), "[1---]");
    CHECK_EQ(format("[{:#>4}]", "x"), "[###x]");
}

TEST(format_zero_padding_respects_sign_and_prefix)
{
    CHECK_EQ(format("{:05}", 42), "00042");
    CHECK_EQ(format("{:05}", -42), "-0042");
    CHECK_EQ(format("{:#06x}", 255), "0x00ff");
    CHECK_EQ(format("{:+05}", 7), "+0007");
}

TEST(format_sign_options)
{
    CHECK_EQ(format("{:+}", 5), "+5");
    CHECK_EQ(format("{:+}", -5), "-5");
    CHECK_EQ(format("{: }", 5), " 5");
    CHECK_EQ(format("{:+.1f}", 2.0), "+2.0");
}

TEST(format_floats_default_is_shortest_round_trip)
{
    CHECK_EQ(format("{}", 0.5), "0.5");
    CHECK_EQ(format("{}", 3.0), "3");
    CHECK_EQ(format("{}", 0.1), "0.1");
    CHECK_EQ(format("{}", -2.25), "-2.25");
}

TEST(format_floats_fixed_and_scientific)
{
    CHECK_EQ(format("{:.2f}", 3.14159), "3.14");
    CHECK_EQ(format("{:f}", 1.5), "1.500000");
    CHECK_EQ(format("{:8.3f}", 2.5), "   2.500");
    CHECK_EQ(format("{:.2e}", 12345.678), "1.23e+04");
    CHECK_EQ(format("{:.3g}", 0.0001234), "0.000123");
    CHECK_EQ(format("{:08.2f}", -3.14159), "-0003.14");
}

TEST(format_floats_special_values)
{
    CHECK_EQ(format("{}", std::numeric_limits<double>::infinity()), "inf");
    CHECK_EQ(format("{}", -std::numeric_limits<double>::infinity()), "-inf");
    CHECK_EQ(format("{}", std::numeric_limits<double>::quiet_NaN()), "nan");
    CHECK_EQ(format("{:06}", std::numeric_limits<double>::infinity()), "   inf");
}

TEST(format_string_precision_truncates)
{
    CHECK_EQ(format("{:.3}", "abcdef"), "abc");
    CHECK_EQ(format("[{:8.3}]", "abcdef"), "[abc     ]");
    CHECK_EQ(format("{:.10}", "short"), "short");
}

TEST(format_char_as_number_and_back)
{
    CHECK_EQ(format("{:d}", 'A'), "65");
    CHECK_EQ(format("{:c}", 66), "B");
    CHECK_EQ(format("{:c}", 67u), "C");
}

TEST(format_pointers)
{
    int x = 0;
    std::string s = format("{}", static_cast<const void*>(&x));
    CHECK(s.rfind("0x", 0) == 0);
    CHECK_EQ(format("{}", static_cast<const void*>(nullptr)), "0x0");
}

TEST(format_enums_use_underlying_value)
{
    CHECK_EQ(format("{}", Colour::Green), "2");
}

TEST(format_streamable_user_types)
{
    CHECK_EQ(format("p = {}", Point{3, -4}), "p = (3, -4)");
    CHECK_EQ(format("[{:>10}]", Point{1, 2}), "[    (1, 2)]");
}

TEST(try_format_reports_unmatched_open_brace)
{
    auto r = try_format("abc {", 1);
    CHECK(!r.has_value());
    CHECK(r.error() == (fretwork::FormatError{FormatErrorKind::UnmatchedBrace, 4}));
}

TEST(try_format_reports_unmatched_close_brace)
{
    auto r = try_format("abc }");
    CHECK(!r.has_value());
    CHECK(r.error().kind == FormatErrorKind::UnmatchedBrace);
    CHECK_EQ(r.error().position, 4u);
}

TEST(try_format_reports_missing_arguments)
{
    auto r = try_format("{} {}", 1);
    CHECK(!r.has_value());
    CHECK(r.error().kind == FormatErrorKind::TooFewArguments);
    auto indexed = try_format("{3}", 1);
    CHECK(indexed.error().kind == FormatErrorKind::TooFewArguments);
}

TEST(try_format_reports_bad_index)
{
    auto r = try_format("{x}", 1);
    CHECK(r.error().kind == FormatErrorKind::BadIndex);
}

TEST(try_format_rejects_mixed_indexing)
{
    CHECK(try_format("{} {0}", 1, 2).error().kind == FormatErrorKind::MixedIndexing);
    CHECK(try_format("{0} {}", 1, 2).error().kind == FormatErrorKind::MixedIndexing);
}

TEST(try_format_rejects_bad_specs)
{
    CHECK(try_format("{:5.}", 1.0).error().kind == FormatErrorKind::BadSpec);
    CHECK(try_format("{:5zz}", 1).error().kind == FormatErrorKind::BadSpec);
    CHECK(try_format("{:9999999}", 1).error().kind == FormatErrorKind::BadSpec);
}

TEST(try_format_rejects_types_that_do_not_fit)
{
    CHECK(try_format("{:q}", 1).error().kind == FormatErrorKind::BadType);
    CHECK(try_format("{:d}", "text").error().kind == FormatErrorKind::BadType);
    CHECK(try_format("{:x}", 1.5).error().kind == FormatErrorKind::BadType);
}

TEST(format_throws_on_error)
{
    CHECK_THROWS(format("{}"), fretwork::FormatException);
    bool kind_ok = false;
    try {
        (void)format("{");
    } catch (const fretwork::FormatException& e) {
        kind_ok = e.error().kind == FormatErrorKind::UnmatchedBrace;
    }
    CHECK(kind_ok);
}

TEST(format_extra_arguments_are_ignored)
{
    CHECK_EQ(format("{}", 1, 2, 3), "1");
}

TEST(format_a_table_row)
{
    CHECK_EQ(format("|{:<8}|{:>6.2f}|{:^5}|", "widget", 3.14159, 'x'), "|widget  |  3.14|  x  |");
}
