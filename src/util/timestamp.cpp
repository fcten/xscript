#include "timestamp.hpp"

namespace xscript::util {

timestamp::timestamp() :
    d(std::chrono::system_clock::now().time_since_epoch())
{

}

// 转为秒级时间戳
std::time_t timestamp::seconds() {
    std::chrono::seconds ts = std::chrono::duration_cast<std::chrono::seconds>(d);
    return ts.count();
}

// 转为毫秒级时间戳
std::time_t timestamp::milliseconds() {
    std::chrono::milliseconds ts = std::chrono::duration_cast<std::chrono::milliseconds>(d);
    return ts.count();
}

// 转为微秒级时间戳
std::time_t timestamp::microseconds() {
    std::chrono::microseconds ts = std::chrono::duration_cast<std::chrono::microseconds>(d);
    return ts.count();
}

// 转为纳秒级时间戳
std::time_t timestamp::nanoseconds() {
    std::chrono::nanoseconds ts = std::chrono::duration_cast<std::chrono::nanoseconds>(d);
    return ts.count();
}

std::string format() {
    return "";
}

}