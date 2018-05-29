aux_source_directory(. DIR_TOKENIZER_SRCS)
add_library(tokenizer ${DIR_TOKENIZER_SRCS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})