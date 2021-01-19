#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include "syntax.hpp"
#include "../util/log.hpp"

namespace xscript::parser {

syntax::syntax() {

}

bool syntax::load(std::string path) {
    auto it = packages.find(path);
    if (it != packages.end()) {
        return true;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout <<  "[" COLOR_RED "error" COLOR_RESET "] can't open source file: " COLOR_WHITE << path <<  COLOR_RESET << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    packages[path] = std::make_unique<ast>(buffer.str());
    if (!packages[path]->parse()) {
        // TODO remove debug code
        packages[path]->print();

        packages.erase(path);
        return false;
    }

    // TODO remove debug code
    packages[path]->print();

    return true;
}

bool syntax::reload(std::string path) {
    packages.erase(path);
    return load(path);
}



}
