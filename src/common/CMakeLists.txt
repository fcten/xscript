aux_source_directory(. DIR_COMMON_SRCS)
add_library(common ${DIR_COMMON_SRCS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})