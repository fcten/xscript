include_directories(${PROJECT_SOURCE_DIR}/src)

aux_source_directory(. LGX_SRCS)

add_executable(x ${LGX_SRCS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

