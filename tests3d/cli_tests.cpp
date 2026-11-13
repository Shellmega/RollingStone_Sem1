#include "cli_options.h"
#include <functional>
#include <iostream>
#include <limits>
int main() {
    int failures=0;
    auto rejects=[&](const std::function<void()>& check) {
        try { check(); ++failures; } catch(const std::exception&) {}
    };
    using namespace rs3d::cli;
    if(integer(L"18446744073709551615")!=std::numeric_limits<std::uint64_t>::max())++failures;
    rejects([]{integer(L"18446744073709551616");});
    rejects([]{integer(L"-1");}); rejects([]{integer(L"2junk");});
    for(const auto* invalid:{L"nan",L"inf",L"1.25junk",L"0",L"8"})
        rejects([&]{real(invalid,.5,3,"scale");});
    if(real(L"1.25",.5,3,"scale")!=1.25)++failures;
    auto size=windowSize(L"1366x768");if(size.first!=1366 || size.second!=768)++failures;
    for(const auto* invalid:{L"1366x768junk",L"0x0",L"10x10",L"1366x",L"1366x768x2"})
        rejects([&]{windowSize(invalid);});
    if(failures) { std::cerr<<failures<<" CLI validation failures\n";return 1; }
    std::cout<<"CLI strict parsing tests passed\n";return 0;
}
