#pragma once
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace rs3d::cli {
inline std::uint64_t integer(const std::wstring& text) {
    if(text.empty() || text.find_first_not_of(L"0123456789")!=text.npos)
        throw std::invalid_argument("expected an unsigned integer");
    return std::stoull(text);
}
inline double real(const std::wstring& text,double minimum,double maximum,const char* name) {
    size_t used=0; double value=std::stod(text,&used);
    if(used!=text.size() || !std::isfinite(value) || value<minimum || value>maximum)
        throw std::invalid_argument(std::string(name)+": invalid finite number or range");
    return value;
}
inline std::pair<int,int> windowSize(const std::wstring& text) {
    auto split=text.find(L'x');
    if(split==text.npos)throw std::invalid_argument("size must be WIDTHxHEIGHT");
    auto width=integer(text.substr(0,split)),height=integer(text.substr(split+1));
    if(width<640 || width>16384 || height<480 || height>16384)
        throw std::invalid_argument("window size must be 640..16384 by 480..16384 pixels");
    return {int(width),int(height)};
}
}
