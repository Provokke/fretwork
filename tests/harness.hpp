#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace harness {

struct Case {
    std::string name;
    std::function<void()> body;
};

struct Failure {
    std::string message;
};

inline std::vector<Case>& registry()
{
    static std::vector<Case> cases;
    return cases;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> body) { registry().push_back({name, std::move(body)}); }
};

inline int run(const std::string& filter)
{
    int ran = 0;
    int failed = 0;
    for (const auto& c : registry()) {
        if (!filter.empty() && c.name.find(filter) == std::string::npos) {
            continue;
        }
        ran++;
        try {
            c.body();
            std::cout << "ok   " << c.name << "\n";
        } catch (const Failure& f) {
            failed++;
            std::cout << "FAIL " << c.name << "\n     " << f.message << "\n";
        } catch (const std::exception& e) {
            failed++;
            std::cout << "FAIL " << c.name << "\n     unexpected exception: " << e.what() << "\n";
        }
    }
    std::cout << ran << " tests, " << (ran - failed) << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}

}

#define HARNESS_CAT2(a, b) a##b
#define HARNESS_CAT(a, b) HARNESS_CAT2(a, b)

#define TEST(name)                                                                  \
    static void name();                                                             \
    static ::harness::Registrar HARNESS_CAT(registrar_, name)(#name, name);         \
    static void name()

#define CHECK(expr)                                                                                    \
    do {                                                                                               \
        if (!(expr)) {                                                                                 \
            throw ::harness::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) +          \
                                     ": check failed: " + #expr};                                      \
        }                                                                                              \
    } while (0)

#define CHECK_EQ(a, b)                                                                                 \
    do {                                                                                               \
        auto harness_a = (a);                                                                        \
        auto harness_b = (b);                                                                        \
        if (!(harness_a == harness_b)) {                                                               \
            throw ::harness::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) +          \
                                     ": " + #a + " != " + #b};                                         \
        }                                                                                              \
    } while (0)

#define CHECK_THROWS(expr, type)                                                                       \
    do {                                                                                               \
        bool harness_thrown = false;                                                                   \
        try {                                                                                          \
            (void)(expr);                                                                              \
        } catch (const type&) {                                                                        \
            harness_thrown = true;                                                                     \
        }                                                                                              \
        if (!harness_thrown) {                                                                         \
            throw ::harness::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) +          \
                                     ": expected " #type " from " #expr};                              \
        }                                                                                              \
    } while (0)
