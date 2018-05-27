# Add block directories
add_subdirectory(common)

# Target
add_executable(interpreter interpreter.c)
target_link_libraries(interpreter common)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})