set(INTERPRETER_SRCS interpreter.c)

# Add block directories
aux_source_directory(common INTERPRETER_SRCS)
aux_source_directory(tokenizer INTERPRETER_SRCS)
aux_source_directory(parser INTERPRETER_SRCS)

# Target
add_executable(interpreter ${INTERPRETER_SRCS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})