#pragma once

#include <exception>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace fretwork {

template <class E>
class Unexpected {
public:
    constexpr explicit Unexpected(E error) : error_(std::move(error)) {}

    constexpr const E& error() const& noexcept { return error_; }
    constexpr E& error() & noexcept { return error_; }
    constexpr E&& error() && noexcept { return std::move(error_); }

    friend constexpr bool operator==(const Unexpected& a, const Unexpected& b) { return a.error_ == b.error_; }

private:
    E error_;
};

template <class E>
Unexpected(E) -> Unexpected<E>;

template <class E>
constexpr Unexpected<std::decay_t<E>> unexpected(E&& error)
{
    return Unexpected<std::decay_t<E>>(std::forward<E>(error));
}

template <class E>
class BadExpectedAccess : public std::exception {
public:
    explicit BadExpectedAccess(E error) : error_(std::move(error)) {}

    const char* what() const noexcept override { return "bad expected access"; }
    const E& error() const noexcept { return error_; }

private:
    E error_;
};

template <class T, class E>
class Expected;

namespace detail {

template <class>
struct is_expected : std::false_type {};

template <class T, class E>
struct is_expected<Expected<T, E>> : std::true_type {};

template <class T>
inline constexpr bool is_expected_v = is_expected<std::remove_cvref_t<T>>::value;

}

template <class T, class E>
class Expected {
public:
    using value_type = T;
    using error_type = E;

    constexpr Expected() requires std::is_default_constructible_v<T> : state_(std::in_place_index<0>) {}

    template <class U = T>
        requires(std::is_constructible_v<T, U &&> && !std::is_same_v<std::remove_cvref_t<U>, Expected> &&
                 !std::is_same_v<std::remove_cvref_t<U>, Unexpected<E>>)
    constexpr Expected(U&& value) : state_(std::in_place_index<0>, std::forward<U>(value))
    {
    }

    template <class G>
        requires std::is_constructible_v<E, G>
    constexpr Expected(Unexpected<G> error) : state_(std::in_place_index<1>, std::move(error).error())
    {
    }

    constexpr bool has_value() const noexcept { return state_.index() == 0; }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr T& operator*() & noexcept { return *std::get_if<0>(&state_); }
    constexpr const T& operator*() const& noexcept { return *std::get_if<0>(&state_); }
    constexpr T&& operator*() && noexcept { return std::move(*std::get_if<0>(&state_)); }
    constexpr T* operator->() noexcept { return std::get_if<0>(&state_); }
    constexpr const T* operator->() const noexcept { return std::get_if<0>(&state_); }

    T& value() &
    {
        if (!has_value()) {
            throw BadExpectedAccess<E>(error());
        }
        return **this;
    }

    const T& value() const&
    {
        if (!has_value()) {
            throw BadExpectedAccess<E>(error());
        }
        return **this;
    }

    T&& value() &&
    {
        if (!has_value()) {
            throw BadExpectedAccess<E>(error());
        }
        return std::move(**this);
    }

    constexpr E& error() & noexcept { return *std::get_if<1>(&state_); }
    constexpr const E& error() const& noexcept { return *std::get_if<1>(&state_); }
    constexpr E&& error() && noexcept { return std::move(*std::get_if<1>(&state_)); }

    template <class U>
    constexpr T value_or(U&& fallback) const&
    {
        return has_value() ? **this : static_cast<T>(std::forward<U>(fallback));
    }

    template <class U>
    constexpr T value_or(U&& fallback) &&
    {
        return has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(fallback));
    }

    template <class F>
    constexpr auto map(F&& f) const&
    {
        using R = std::remove_cv_t<std::invoke_result_t<F, const T&>>;
        if (has_value()) {
            return Expected<R, E>(std::invoke(std::forward<F>(f), **this));
        }
        return Expected<R, E>(unexpected(error()));
    }

    template <class F>
    constexpr auto map(F&& f) &&
    {
        using R = std::remove_cv_t<std::invoke_result_t<F, T&&>>;
        if (has_value()) {
            return Expected<R, E>(std::invoke(std::forward<F>(f), std::move(**this)));
        }
        return Expected<R, E>(unexpected(std::move(error())));
    }

    template <class F>
    constexpr auto map_error(F&& f) const&
    {
        using G = std::remove_cv_t<std::invoke_result_t<F, const E&>>;
        if (has_value()) {
            return Expected<T, G>(**this);
        }
        return Expected<T, G>(unexpected(std::invoke(std::forward<F>(f), error())));
    }

    template <class F>
    constexpr auto and_then(F&& f) const&
    {
        using R = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        static_assert(detail::is_expected_v<R>, "and_then needs a callable returning Expected");
        if (has_value()) {
            return std::invoke(std::forward<F>(f), **this);
        }
        return R(unexpected(error()));
    }

    template <class F>
    constexpr auto and_then(F&& f) &&
    {
        using R = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        static_assert(detail::is_expected_v<R>, "and_then needs a callable returning Expected");
        if (has_value()) {
            return std::invoke(std::forward<F>(f), std::move(**this));
        }
        return R(unexpected(std::move(error())));
    }

    template <class F>
    constexpr Expected or_else(F&& f) const&
    {
        if (has_value()) {
            return *this;
        }
        return std::invoke(std::forward<F>(f), error());
    }

    constexpr std::optional<T> to_optional() const&
    {
        if (has_value()) {
            return **this;
        }
        return std::nullopt;
    }

    friend constexpr bool operator==(const Expected& a, const Expected& b)
    {
        if (a.has_value() != b.has_value()) {
            return false;
        }
        return a.has_value() ? *a == *b : a.error() == b.error();
    }

    friend constexpr bool operator==(const Expected& a, const T& b) { return a.has_value() && *a == b; }

    template <class G>
    friend constexpr bool operator==(const Expected& a, const Unexpected<G>& b)
    {
        return !a.has_value() && a.error() == b.error();
    }

private:
    std::variant<T, E> state_;
};

template <class T, class E>
Expected<T, E> from_optional(std::optional<T> value, E error)
{
    if (value) {
        return Expected<T, E>(std::move(*value));
    }
    return Expected<T, E>(unexpected(std::move(error)));
}

template <class T, class E>
Expected<std::vector<T>, E> collect(const std::vector<Expected<T, E>>& items)
{
    std::vector<T> out;
    out.reserve(items.size());
    for (const auto& item : items) {
        if (!item) {
            return Expected<std::vector<T>, E>(unexpected(item.error()));
        }
        out.push_back(*item);
    }
    return Expected<std::vector<T>, E>(std::move(out));
}

}

