#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include "syntax.hpp"

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
        // TODO report error
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    packages[path] = std::make_unique<ast>(buffer.str());
    if (!packages[path]->parse()) {
        packages.erase(path);
        return false;
    }

    return true;
}

bool syntax::reload(std::string path) {
    packages.erase(path);
    return load(path);
}



}
