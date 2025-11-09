#include "fretwork/expected.hpp"
#include "fretwork/format.hpp"
#include "fretwork/optional.hpp"
#include "fretwork/strings.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace fretwork;

struct Config {
    int port;
    double ratio;
};

Expected<Config, std::string> load(std::string_view port_text, std::string_view ratio_text)
{
    auto port = parse<int>(port_text).map_error([&](ParseError) { return format("bad port '{}'", port_text); });
    if (!port) {
        return unexpected(port.error());
    }
    auto ratio = parse<double>(ratio_text).map_error([&](ParseError) { return format("bad ratio '{}'", ratio_text); });
    if (!ratio) {
        return unexpected(ratio.error());
    }
    return Config{*port, *ratio};
}

int main()
{
    for (auto [p, r] : std::vector<std::pair<std::string, std::string>>{{"8080", "0.75"}, {"80x", "1"}, {"443", "?"}}) {
        auto cfg = load(p, r);
        if (cfg) {
            std::cout << format("port={:>5} ratio={:.2f}\n", cfg->port, cfg->ratio);
        } else {
            std::cout << format("error: {}\n", cfg.error());
        }
    }

    std::string label = std::optional<std::string>(" Widget ") | map([](const std::string& s) { return std::string(trim(s)); }) |
                        filter([](const std::string& s) { return !s.empty(); }) | map([](const std::string& s) { return to_upper(s); }) |
                        value_or(std::string("unnamed"));
    std::cout << format("[{:^12}]\n", label);
    std::cout << format("{:#06x} {:b} {:+d}\n", 255, 10, 7);
}
