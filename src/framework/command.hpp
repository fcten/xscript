#ifndef _XSCRIPT_FRAMEWORK_COMMAND_HPP_
#define _XSCRIPT_FRAMEWORK_COMMAND_HPP_

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace xscript::framework {

class command {
private:
    std::vector<std::string> source_files;

public:
    void init(int argc, char* argv[]);
    
    const std::vector<std::string>& get_source_files();
};

}

#endif // _XSCRIPT_FRAMEWORK_COMMAND_HPP_