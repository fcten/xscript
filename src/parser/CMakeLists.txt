aux_source_directory(. DIR_PARSER_SRCS)
add_library(parser ${DIR_PARSER_SRCS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})