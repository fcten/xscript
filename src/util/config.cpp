#include "config.hpp"

namespace xscript::util {

std::string config::get_env(std::string key) {
    return get_env(key, "");
}

std::string config::get_env(std::string key, std::string default_value) {
    auto it = envs.find(key);
    if (it == envs.end()) {
        return default_value;
    }
    return it->second;
}

}
