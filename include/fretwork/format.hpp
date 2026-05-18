#pragma once

#include "expected.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace fretwork {

enum class FormatErrorKind {
    UnmatchedBrace,
    BadIndex,
    TooFewArguments,
    BadSpec,
    BadType,
    MixedIndexing,
};

struct FormatError {
    FormatErrorKind kind;
    std::size_t position;

    friend bool operator==(const FormatError& a, const FormatError& b)
    {
        return a.kind == b.kind && a.position == b.position;
    }
};

class FormatException : public std::runtime_error {
public:
    explicit FormatException(FormatError error)
        : std::runtime_error("format error at position " + std::to_string(error.position)), error_(error)
    {
    }

    const FormatError& error() const noexcept { return error_; }

private:
    FormatError error_;
};

namespace detail {

struct Spec {
    char fill = ' ';
    char align = 0;
    char sign = '-';
    bool alt = false;
    bool zero = false;
    std::size_t width = 0;
    int precision = -1;
    char type = 0;
};

using Arg = std::variant<long long, unsigned long long, double, bool, char, std::string_view, const void*, std::string>;

template <class T>
concept Streamable = requires(std::ostream& os, const T& v) { os << v; };

template <class T>
Arg make_arg(const T& value)
{
    using D = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<D, bool>) {
        return Arg(std::in_place_type<bool>, value);
    } else if constexpr (std::is_same_v<D, char>) {
        return Arg(std::in_place_type<char>, value);
    } else if constexpr (std::is_integral_v<D> && std::is_signed_v<D>) {
        return Arg(std::in_place_type<long long>, static_cast<long long>(value));
    } else if constexpr (std::is_integral_v<D>) {
        return Arg(std::in_place_type<unsigned long long>, static_cast<unsigned long long>(value));
    } else if constexpr (std::is_enum_v<D>) {
        return make_arg(static_cast<std::underlying_type_t<D>>(value));
    } else if constexpr (std::is_floating_point_v<D>) {
        return Arg(std::in_place_type<double>, static_cast<double>(value));
    } else if constexpr (std::is_convertible_v<const D&, std::string_view>) {
        return Arg(std::in_place_type<std::string_view>, std::string_view(value));
    } else if constexpr (std::is_pointer_v<D> || std::is_null_pointer_v<D>) {
        return Arg(std::in_place_type<const void*>, static_cast<const void*>(value));
    } else {
        static_assert(Streamable<D>, "argument type is not formattable");
        std::ostringstream os;
        os << value;
        return Arg(std::in_place_type<std::string>, os.str());
    }
}

inline bool is_align(char c)
{
    return c == '<' || c == '>' || c == '^';
}

inline Expected<Spec, FormatError> parse_spec(std::string_view s, std::size_t base)
{
    Spec spec;
    std::size_t i = 0;
    auto bad = [&](std::size_t at) { return Expected<Spec, FormatError>(unexpected(FormatError{FormatErrorKind::BadSpec, base + at})); };
    if (s.size() >= 2 && is_align(s[1])) {
        spec.fill = s[0];
        spec.align = s[1];
        i = 2;
    } else if (!s.empty() && is_align(s[0])) {
        spec.align = s[0];
        i = 1;
    }
    if (i < s.size() && (s[i] == '+' || s[i] == '-' || s[i] == ' ')) {
        spec.sign = s[i++];
    }
    if (i < s.size() && s[i] == '#') {
        spec.alt = true;
        i++;
    }
    if (i < s.size() && s[i] == '0') {
        spec.zero = true;
        i++;
    }
    std::size_t width = 0;
    bool has_width = false;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
        width = width * 10 + static_cast<std::size_t>(s[i] - '0');
        if (width > 100000) {
            return bad(i);
        }
        has_width = true;
        i++;
    }
    if (has_width) {
        spec.width = width;
    }
    if (i < s.size() && s[i] == '.') {
        i++;
        int precision = 0;
        bool digits = false;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            precision = precision * 10 + (s[i] - '0');
            if (precision > 1000) {
                return bad(i);
            }
            digits = true;
            i++;
        }
        if (!digits) {
            return bad(i);
        }
        spec.precision = precision;
    }
    if (i < s.size()) {
        spec.type = s[i++];
    }
    if (i != s.size()) {
        return bad(i);
    }
    return spec;
}

inline std::string pad(std::string body, const Spec& spec, char default_align, std::size_t prefix_len = 0)
{
    if (body.size() >= spec.width) {
        return body;
    }
    std::size_t gap = spec.width - body.size();
    if (spec.zero && spec.align == 0 && default_align == '>') {
        body.insert(prefix_len, gap, '0');
        return body;
    }
    char align = spec.align ? spec.align : default_align;
    if (align == '<') {
        body.append(gap, spec.fill);
    } else if (align == '>') {
        body.insert(0, gap, spec.fill);
    } else {
        std::size_t left = gap / 2;
        body.insert(0, left, spec.fill);
        body.append(gap - left, spec.fill);
    }
    return body;
}

inline std::string sign_prefix(bool negative, const Spec& spec)
{
    if (negative) {
        return "-";
    }
    if (spec.sign == '+') {
        return "+";
    }
    if (spec.sign == ' ') {
        return " ";
    }
    return "";
}

inline Expected<std::string, FormatError> format_integer(unsigned long long magnitude, bool negative, const Spec& spec, std::size_t at)
{
    int base = 10;
    bool upper = false;
    std::string prefix;
    switch (spec.type) {
    case 0:
    case 'd':
        break;
    case 'x':
        base = 16;
        break;
    case 'X':
        base = 16;
        upper = true;
        break;
    case 'b':
        base = 2;
        break;
    case 'o':
        base = 8;
        break;
    default:
        return unexpected(FormatError{FormatErrorKind::BadType, at});
    }
    if (spec.alt) {
        prefix = base == 16 ? (upper ? "0X" : "0x") : base == 2 ? "0b" : base == 8 ? "0" : "";
    }
    std::array<char, 72> buf{};
    auto res = std::to_chars(buf.data(), buf.data() + buf.size(), magnitude, base);
    std::string digits(buf.data(), res.ptr);
    if (upper) {
        for (char& c : digits) {
            if (c >= 'a' && c <= 'f') {
                c = static_cast<char>(c - 'a' + 'A');
            }
        }
    }
    std::string head = sign_prefix(negative, spec) + prefix;
    return pad(head + digits, spec, '>', head.size());
}

inline Expected<std::string, FormatError> format_double(double value, const Spec& spec, std::size_t at)
{
    bool negative = std::signbit(value);
    double mag = negative ? -value : value;
    std::string body;
    if (std::isnan(value)) {
        body = "nan";
        negative = false;
    } else if (std::isinf(value)) {
        body = "inf";
    } else {
        std::array<char, 400> buf{};
        std::to_chars_result res{};
        switch (spec.type) {
        case 0:
            if (spec.precision < 0) {
                res = std::to_chars(buf.data(), buf.data() + buf.size(), mag);
            } else {
                res = std::to_chars(buf.data(), buf.data() + buf.size(), mag, std::chars_format::general, spec.precision);
            }
            break;
        case 'f':
            res = std::to_chars(buf.data(), buf.data() + buf.size(), mag, std::chars_format::fixed, spec.precision < 0 ? 6 : spec.precision);
            break;
        case 'e':
            res = std::to_chars(buf.data(), buf.data() + buf.size(), mag, std::chars_format::scientific, spec.precision < 0 ? 6 : spec.precision);
            break;
        case 'g':
            res = std::to_chars(buf.data(), buf.data() + buf.size(), mag, std::chars_format::general, spec.precision < 0 ? 6 : spec.precision);
            break;
        default:
            return unexpected(FormatError{FormatErrorKind::BadType, at});
        }
        if (res.ec != std::errc()) {
            return unexpected(FormatError{FormatErrorKind::BadSpec, at});
        }
        body.assign(buf.data(), res.ptr);
    }
    std::string head = sign_prefix(negative, spec);
    bool finite = std::isfinite(value);
    Spec local = spec;
    if (!finite) {
        local.zero = false;
    }
    return pad(head + body, local, '>', head.size());
}

inline Expected<std::string, FormatError> format_string(std::string_view text, const Spec& spec, std::size_t at)
{
    if (spec.type != 0 && spec.type != 's') {
        return unexpected(FormatError{FormatErrorKind::BadType, at});
    }
    if (spec.precision >= 0 && static_cast<std::size_t>(spec.precision) < text.size()) {
        text = text.substr(0, static_cast<std::size_t>(spec.precision));
    }
    Spec local = spec;
    local.zero = false;
    return pad(std::string(text), local, '<');
}

inline Expected<std::string, FormatError> format_arg(const Arg& arg, const Spec& spec, std::size_t at)
{
    switch (arg.index()) {
    case 0: {
        long long v = std::get<0>(arg);
        unsigned long long mag = v < 0 ? 0ull - static_cast<unsigned long long>(v) : static_cast<unsigned long long>(v);
        if (spec.type == 'c') {
            return format_string(std::string(1, static_cast<char>(v)), Spec{spec.fill, spec.align}, at);
        }
        return format_integer(mag, v < 0, spec, at);
    }
    case 1:
        if (spec.type == 'c') {
            return format_string(std::string(1, static_cast<char>(std::get<1>(arg))), Spec{spec.fill, spec.align}, at);
        }
        return format_integer(std::get<1>(arg), false, spec, at);
    case 2:
        return format_double(std::get<2>(arg), spec, at);
    case 3:
        if (spec.type == 0 || spec.type == 's') {
            return format_string(std::get<3>(arg) ? "true" : "false", spec, at);
        }
        return format_integer(std::get<3>(arg) ? 1 : 0, false, spec, at);
    case 4:
        if (spec.type == 0 || spec.type == 's' || spec.type == 'c') {
            return format_string(std::string(1, std::get<4>(arg)), spec, at);
        }
        return format_integer(static_cast<unsigned char>(std::get<4>(arg)), false, spec, at);
    case 5:
        return format_string(std::get<5>(arg), spec, at);
    case 6: {
        Spec local = spec;
        local.alt = true;
        local.type = 'x';
        return format_integer(reinterpret_cast<std::uintptr_t>(std::get<6>(arg)), false, local, at);
    }
    default:
        return format_string(std::get<7>(arg), spec, at);
    }
}

inline Expected<std::string, FormatError> format_impl(std::string_view fmt, const Arg* args, std::size_t count)
{
    std::string out;
    out.reserve(fmt.size() + 16);
    std::size_t next_auto = 0;
    int mode = 0;
    std::size_t i = 0;
    while (i < fmt.size()) {
        char c = fmt[i];
        if (c == '}') {
            if (i + 1 < fmt.size() && fmt[i + 1] == '}') {
                out.push_back('}');
                i += 2;
                continue;
            }
            return unexpected(FormatError{FormatErrorKind::UnmatchedBrace, i});
        }
        if (c != '{') {
            out.push_back(c);
            i++;
            continue;
        }
        if (i + 1 < fmt.size() && fmt[i + 1] == '{') {
            out.push_back('{');
            i += 2;
            continue;
        }
        std::size_t open = i;
        std::size_t close = fmt.find('}', i + 1);
        if (close == std::string_view::npos) {
            return unexpected(FormatError{FormatErrorKind::UnmatchedBrace, open});
        }
        std::string_view field = fmt.substr(open + 1, close - open - 1);
        std::size_t colon = field.find(':');
        std::string_view index_text = field.substr(0, colon);
        std::string_view spec_text = colon == std::string_view::npos ? std::string_view() : field.substr(colon + 1);
        std::size_t index = 0;
        if (index_text.empty()) {
            if (mode == 2) {
                return unexpected(FormatError{FormatErrorKind::MixedIndexing, open});
            }
            mode = 1;
            index = next_auto++;
        } else {
            if (mode == 1) {
                return unexpected(FormatError{FormatErrorKind::MixedIndexing, open});
            }
            mode = 2;
            auto res = std::from_chars(index_text.data(), index_text.data() + index_text.size(), index);
            if (res.ec != std::errc() || res.ptr != index_text.data() + index_text.size()) {
                return unexpected(FormatError{FormatErrorKind::BadIndex, open + 1});
            }
        }
        if (index >= count) {
            return unexpected(FormatError{FormatErrorKind::TooFewArguments, open});
        }
        auto spec = parse_spec(spec_text, colon == std::string_view::npos ? close : open + 1 + colon + 1);
        if (!spec) {
            return unexpected(spec.error());
        }
        auto piece = format_arg(args[index], *spec, open);
        if (!piece) {
            return unexpected(piece.error());
        }
        out += *piece;
        i = close + 1;
    }
    return out;
}

}

template <class... Args>
Expected<std::string, FormatError> try_format(std::string_view fmt, const Args&... args)
{
    if constexpr (sizeof...(Args) == 0) {
        return detail::format_impl(fmt, nullptr, 0);
    } else {
        const detail::Arg packed[] = {detail::make_arg(args)...};
        return detail::format_impl(fmt, packed, sizeof...(Args));
    }
}

template <class... Args>
std::string format(std::string_view fmt, const Args&... args)
{
    auto result = try_format(fmt, args...);
    if (!result) {
        throw FormatException(result.error());
    }
    return std::move(result).value();
}

}

