#pragma once

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace fretwork {

namespace detail {

template <class F>
struct MapStep {
    F f;
};

template <class F>
struct AndThenStep {
    F f;
};

template <class F>
struct OrElseStep {
    F f;
};

template <class F>
struct FilterStep {
    F f;
};

template <class T>
struct ValueOrStep {
    T fallback;
};

template <class F>
struct TapStep {
    F f;
};

}

template <class F>
constexpr detail::MapStep<std::decay_t<F>> map(F&& f)
{
    return {std::forward<F>(f)};
}

template <class F>
constexpr detail::AndThenStep<std::decay_t<F>> and_then(F&& f)
{
    return {std::forward<F>(f)};
}

template <class F>
constexpr detail::OrElseStep<std::decay_t<F>> or_else(F&& f)
{
    return {std::forward<F>(f)};
}

template <class F>
constexpr detail::FilterStep<std::decay_t<F>> filter(F&& f)
{
    return {std::forward<F>(f)};
}

template <class F>
constexpr detail::TapStep<std::decay_t<F>> tap(F&& f)
{
    return {std::forward<F>(f)};
}

template <class T>
constexpr detail::ValueOrStep<std::decay_t<T>> value_or(T&& fallback)
{
    return {std::forward<T>(fallback)};
}

template <class T, class F>
constexpr auto operator|(std::optional<T> opt, detail::MapStep<F> step)
{
    using R = std::remove_cv_t<std::invoke_result_t<F, T&&>>;
    if (opt) {
        return std::optional<R>(std::invoke(std::move(step.f), std::move(*opt)));
    }
    return std::optional<R>();
}

template <class T, class F>
constexpr auto operator|(std::optional<T> opt, detail::AndThenStep<F> step)
{
    using R = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
    if (opt) {
        return std::invoke(std::move(step.f), std::move(*opt));
    }
    return R();
}

template <class T, class F>
constexpr std::optional<T> operator|(std::optional<T> opt, detail::OrElseStep<F> step)
{
    if (opt) {
        return opt;
    }
    return std::invoke(std::move(step.f));
}

template <class T, class F>
constexpr std::optional<T> operator|(std::optional<T> opt, detail::FilterStep<F> step)
{
    if (opt && std::invoke(std::move(step.f), std::as_const(*opt))) {
        return opt;
    }
    return std::nullopt;
}

template <class T, class F>
constexpr std::optional<T> operator|(std::optional<T> opt, detail::TapStep<F> step)
{
    if (opt) {
        std::invoke(std::move(step.f), std::as_const(*opt));
    }
    return opt;
}

template <class T, class U>
constexpr T operator|(std::optional<T> opt, detail::ValueOrStep<U> step)
{
    return opt ? std::move(*opt) : static_cast<T>(std::move(step.fallback));
}

template <class T, class... Rest>
constexpr std::optional<T> first_some(std::optional<T> head, Rest&&... rest)
{
    if (head) {
        return head;
    }
    if constexpr (sizeof...(rest) == 0) {
        return std::nullopt;
    } else {
        return first_some(std::forward<Rest>(rest)...);
    }
}

template <class T>
constexpr std::optional<T> some_if(bool condition, T value)
{
    if (condition) {
        return std::optional<T>(std::move(value));
    }
    return std::nullopt;
}

template <class T>
constexpr std::optional<T> flatten(std::optional<std::optional<T>> nested)
{
    if (nested) {
        return std::move(*nested);
    }
    return std::nullopt;
}

}
