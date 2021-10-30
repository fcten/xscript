#ifndef _XSCRIPT_UTIL_TIMESTAMP_HPP_
#define _XSCRIPT_UTIL_TIMESTAMP_HPP_

#include <ctime>
#include <string>
#include <chrono>

namespace xscript::util {

class timestamp {
private:
    std::chrono::system_clock::duration d;

public:
    // 无参构造函数，默认初始化为当前时间
    timestamp();

    // 转为秒级时间戳
    std::time_t seconds();

    // 转为毫秒级时间戳
    std::time_t milliseconds();

    // 转为微秒级时间戳
    std::time_t microseconds();

    // 转为纳秒级时间戳
    std::time_t nanoseconds();

    std::string format();
}


}

#endif // _XSCRIPT_UTIL_TIMESTAMP_HPP_