set(XSCRIPT_SRCS xscript.c)

# Add block directories
aux_source_directory(common XSCRIPT_SRCS)
aux_source_directory(tokenizer XSCRIPT_SRCS)
aux_source_directory(parser XSCRIPT_SRCS)
aux_source_directory(optimizer XSCRIPT_SRCS)
aux_source_directory(compiler XSCRIPT_SRCS)
aux_source_directory(interpreter XSCRIPT_SRCS)
aux_source_directory(extensions XSCRIPT_SRCS)
aux_source_directory(webit XSCRIPT_SRCS)
aux_source_directory(webit/common XSCRIPT_SRCS)
aux_source_directory(webit/thread XSCRIPT_SRCS)
aux_source_directory(webit/http XSCRIPT_SRCS)

if (UNIX)
    set (XSCRIPT_SRCS ${XSCRIPT_SRCS}
    webit/event/wbt_event.h
    webit/event/wbt_event_epoll.c
    webit/event/wbt_event_epoll.h
	)
	aux_source_directory(webit/os/linux XSCRIPT_SRCS)
endif (UNIX)

if (WIN32 OR CYGWIN)
    set (XSCRIPT_SRCS ${XSCRIPT_SRCS}
    webit/event/wbt_event.h
    webit/event/wbt_event_select.c
    webit/event/wbt_event_select.h
	)
	aux_source_directory(webit/os/win32 XSCRIPT_SRCS)
endif (WIN32 OR CYGWIN)

# Target
add_executable(xscript ${XSCRIPT_SRCS})

if (UNIX)
	set (XSCRIPT_LIBS ${XSCRIPT_LIBS} dl pthread)
endif (UNIX)

if (WIN32)
	set (XSCRIPT_LIBS ${XSCRIPT_LIBS} ws2_32)
endif (WIN32)

target_link_libraries(xscript ${XSCRIPT_LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})