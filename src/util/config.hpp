#ifndef _XSCRIPT_UTIL_CONFIG_HPP_
#define _XSCRIPT_UTIL_CONFIG_HPP_

#include <string>
#include <unordered_map>
#include "singleton.hpp"
#include "env.hpp"

namespace xscript::util {

class config : public singleton<config>  {
private:
    std::unordered_map<std::string, std::string> envs;

public:
    bool init();

    std::string get_env(std::string key);
    std::string get_env(std::string key, std::string default_value);
};

}

#endif // _XSCRIPT_UTIL_CONFIG_HPP_