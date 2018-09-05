set(XSCRIPT_SRCS xscript.c)

# Add block directories
aux_source_directory(common XSCRIPT_SRCS)
aux_source_directory(tokenizer XSCRIPT_SRCS)
aux_source_directory(parser XSCRIPT_SRCS)
aux_source_directory(optimizer XSCRIPT_SRCS)
aux_source_directory(compiler XSCRIPT_SRCS)
aux_source_directory(interpreter XSCRIPT_SRCS)

# Target
add_executable(xscript ${XSCRIPT_SRCS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})