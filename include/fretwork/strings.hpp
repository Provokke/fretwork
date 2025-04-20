#pragma once

#include "expected.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

namespace fretwork {

enum class ParseError {
    Empty,
    Invalid,
    OutOfRange,
};

inline std::string_view trim_left(std::string_view s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    return s;
}

inline std::string_view trim_right(std::string_view s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

inline std::string_view trim(std::string_view s)
{
    return trim_left(trim_right(s));
}

inline std::vector<std::string> split(std::string_view s, std::string_view delimiter)
{
    std::vector<std::string> parts;
    if (delimiter.empty()) {
        parts.emplace_back(s);
        return parts;
    }
    std::size_t start = 0;
    for (;;) {
        std::size_t at = s.find(delimiter, start);
        if (at == std::string_view::npos) {
            parts.emplace_back(s.substr(start));
            return parts;
        }
        parts.emplace_back(s.substr(start, at - start));
        start = at + delimiter.size();
    }
}

template <class Range>
std::string join(const Range& items, std::string_view separator)
{
    std::string out;
    bool first = true;
    for (const auto& item : items) {
        if (!first) {
            out.append(separator);
        }
        first = false;
        out.append(item);
    }
    return out;
}

inline std::string replace_all(std::string_view s, std::string_view from, std::string_view to)
{
    if (from.empty()) {
        return std::string(s);
    }
    std::string out;
    std::size_t start = 0;
    for (;;) {
        std::size_t at = s.find(from, start);
        if (at == std::string_view::npos) {
            out.append(s.substr(start));
            return out;
        }
        out.append(s.substr(start, at - start));
        out.append(to);
        start = at + from.size();
    }
}

inline bool starts_with(std::string_view s, std::string_view prefix)
{
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

inline bool ends_with(std::string_view s, std::string_view suffix)
{
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

inline std::string to_upper(std::string_view s)
{
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

inline std::string to_lower(std::string_view s)
{
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

template <class T>
Expected<T, ParseError> parse(std::string_view s, int base = 10)
{
    s = trim(s);
    if (s.empty()) {
        return unexpected(ParseError::Empty);
    }
    T value{};
    const char* end = s.data() + s.size();
    std::from_chars_result res;
    if constexpr (std::is_floating_point_v<T>) {
        res = std::from_chars(s.data(), end, value);
    } else {
        res = std::from_chars(s.data(), end, value, base);
    }
    if (res.ec == std::errc::result_out_of_range) {
        return unexpected(ParseError::OutOfRange);
    }
    if (res.ec != std::errc() || res.ptr != end) {
        return unexpected(ParseError::Invalid);
    }
    return value;
}

}
