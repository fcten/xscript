# Add block directories
add_subdirectory(common)
add_subdirectory(tokenizer)
add_subdirectory(parser)

# Target
add_executable(interpreter interpreter.c)
target_link_libraries(interpreter common tokenizer parser)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})