#include "command.hpp"

namespace xscript::framework {

void command::init(int argc, char* argv[]) {
    static const struct option long_options[] = {
        {"config", required_argument, NULL, 'c'},
        {"env", required_argument, NULL, 'e'},
        {"daemon", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, no_argument, NULL, 0}
    };

    int c;
    int oi = -1;
    if(argv == NULL) return;
    while((c = getopt_long(argc, argv, ":c:e:dvVh", long_options, &oi)) != -1){
        switch(c) {
        case 'c':
            fprintf(stderr, "-%c %s\n", c, optarg);
            break;
        case 'e':
            fprintf(stderr, "-%c %s\n", c, optarg);
            break;
        case 'd':
            fprintf(stderr, "-%c\n", c);
            break;
        case 'v':
            //_ver_short = true;
            fprintf(stderr, "-%c\n", c);
            exit(1);
            break;
        case 'h':
            fprintf(stderr, "Usage: xscript source_file [options]\n");
            fprintf(stderr, "    -c  --config file_path]\n");
            fprintf(stderr, "    -e  --env name=value]\n");
            fprintf(stderr, "    -d  --daemon\n");
            fprintf(stderr, "    -h  --help\n");
            exit(1);
            break;
        case ':':
            // 参数缺失
            fprintf(stderr, "%s: option '-%c' requires an argument\n", argv[0], optopt);
            break;
        case '?':
            // 非法选项
            fprintf(stderr, "%s: option '-%c' is invalid\n", argv[0], optopt);
            exit(1);
            break;
        default:
            exit(1);
        }
    }

    while (optind < argc) {
        source_files.push_back(argv[optind++]);
    }
}

const std::vector<std::string>& command::get_source_files() {
    return source_files;
}

}
