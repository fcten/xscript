include_directories(${PROJECT_SOURCE_DIR}/src)

aux_source_directory(. LGX_SRCS)

if (UNIX)

endif (UNIX)

if (WIN32 OR CYGWIN)

endif (WIN32 OR CYGWIN)

add_executable(lgx ${LGX_SRCS})

if (UNIX)
#	set (LGX_LIBS ${LGX_LIBS} dl z)
endif (UNIX)

if (WIN32)
#	set (LGX_LIBS ${LGX_LIBS} ws2_32)
endif (WIN32)

target_link_libraries(lgx ${LGX_LIBS})

install(TARGETS lgx RUNTIME DESTINATION ${BINDIR} LIBRARY DESTINATION ${LIBDIR})
install(FILES DESTINATION ${INCLUDEDIR})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

if (UNIX)
	install(CODE "EXEC_PROGRAM(/sbin/ldconfig)")
endif (UNIX)

