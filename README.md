<div align="center">

# fretwork

A header-only C++20 library with three small pieces that go together: an `Expected<T, E>` result type, pipe-style chaining for `std::optional`, and a `{}`-style string formatter that reports errors as values instead of throwing. Everything lives in `include/fretwork`, so there is nothing to link.

![License](https://img.shields.io/badge/LICENSE-MIT-blue?style=for-the-badge)  ![Language](https://img.shields.io/badge/C++20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)  ![Tests](https://img.shields.io/badge/TESTS-74-brightgreen?style=for-the-badge)

Refer to [LICENSE](LICENSE) for the legal bits.

</div>

## What's in it

- **`expected.hpp`**: `Expected<T, E>` and `Unexpected<E>`, written against C++20 because `std::expected` needs C++23.
  - `map`, `map_error`, `and_then`, `or_else`, `value_or`, `to_optional`, with both const-lvalue and rvalue overloads so move-only values work.
  - `value()` throws `BadExpectedAccess<E>` and carries the error payload. `operator*` and `error()` are unchecked and `noexcept`.
  - Usable in `constexpr`, with equality against another `Expected`, a plain value or an `Unexpected`.
  - `from_optional` and `collect` (a vector of results into a result of a vector, stopping at the first error).
- **`optional.hpp`**: pipe adaptors for `std::optional`: `map`, `and_then`, `or_else`, `filter`, `tap`, `value_or`, plus `first_some`, `some_if` and `flatten`.
- **`format.hpp`**: `format` and `try_format`.
  - Automatic or explicit indices (`{}` or `{1}`, not both in one string), `{{` and `}}` escapes.
  - Fill, alignment (`<`, `>`, `^`), sign, `#`, zero padding, width and precision.
  - Types `d x X b o c s f e g`. Floats go through `std::to_chars`, so the default output is the shortest string that round-trips and nothing depends on the locale.
  - Integers, bool, char, enums, floats, strings, pointers, and anything with `operator<<`.
  - `try_format` returns `Expected<std::string, FormatError>` with the kind of failure and where it happened. `format` throws `FormatException` instead.
- **`strings.hpp`**: `trim`, `split`, `join`, `replace_all`, `starts_with`, `ends_with`, case conversion, and `parse<T>` for numbers, which returns `Expected<T, ParseError>`.

```cpp
#include "fretwork/format.hpp"
#include "fretwork/optional.hpp"
#include "fretwork/strings.hpp"

using namespace fretwork;

auto port = parse<int>("8080")
    .and_then([](int p) -> Expected<int, ParseError> {
        return p > 0 && p < 65536 ? Expected<int, ParseError>(p) : unexpected(ParseError::OutOfRange);
    })
    .value_or(80);

std::string label = std::optional<std::string>(" widget ")
    | map([](const std::string& s) { return std::string(trim(s)); })
    | filter([](const std::string& s) { return !s.empty(); })
    | value_or(std::string("unnamed"));

std::string line = format("{:<8}|{:>6.2f}|{:#06x}", label, 3.14159, port);
```

## 🏗️ Architecture

```
 include/fretwork/
  ├─ expected.hpp   Unexpected<E>, Expected<T,E> (std::variant<T,E> inside)
  │                  ▲             ▲
  ├─ optional.hpp    │ (standalone: step objects + operator| on std::optional)
  │                  │
  ├─ strings.hpp ────┘ parse<T> returns Expected<T, ParseError>
  └─ format.hpp ─────── try_format: args ─▶ Arg variant[] ─▶ parse brace field
                                                          ─▶ parse Spec ─▶ format_arg
 tests/    harness.hpp (registry + CHECK macros), main.cpp, one file per header
 examples/ tour.cpp
```

- `Expected` stores a `std::variant<T, E>`, so it needs no manual lifetime management and its special members come for free.
- Optional steps (`map(f)`, `filter(f)` and so on) are small tagged structs. `operator|` is overloaded on each one, which is what lets the chain read left to right.
- The formatter packs each argument into one `Arg` variant up front, then walks the format string once. Spec parsing is separate from rendering, and every failure path returns an `Expected` rather than unwinding.
- `strings.hpp` depends on `expected.hpp` and `format.hpp` depends on it too. They are otherwise independent.

## 🚀 Quick start

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Or run the harness directly. It takes an optional name filter:

```
./build/fretwork_tests format_float
```

`examples/tour.cpp` is built along with the tests and prints:

```
port= 8080 ratio=0.75
error: bad port '80x'
error: bad ratio '?'
[   WIDGET   ]
0x00ff 1010 +7
```

I built and ran all of this in Docker on `gcc:14` with `-Wall -Wextra -Wpedantic -Werror`. To use it in your own project, add `include/` to your include path (or `add_subdirectory` it and link `fretwork::fretwork`).

## 🧪 Testing

74 tests, all passing, on a small self-written harness (`tests/harness.hpp`, no framework):

- Expected (17): value and error access, the throwing `value()`, `value_or`, `map`, `map_error`, `and_then`, `or_else`, equality, move-only payloads, rvalue `map`, `to_optional`, `from_optional`, `collect` and `constexpr` use.
- Optional (13): each adaptor on present and empty values, type-changing `map`, a long mixed chain, `first_some`, `some_if`, `flatten` and `constexpr` chains.
- Format (30): indexing, escapes, every integer radix, alignment and fill, zero padding with signs and prefixes, float formats and special values, string precision, chars, pointers, enums, user types, every `FormatErrorKind`, and the throwing wrapper.
- Strings (14): trimming, splitting (empty fields, multi-character delimiters), joining and round trips, replacing, prefixes, case, and `parse` for integers and doubles including each `ParseError`.
