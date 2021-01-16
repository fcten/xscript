# This is a cmake script. Process it with the CMake gui or command line utility
# to produce makefiles / Visual Studio project files on Mac OS X and Windows.
#
# To configure the build options either use the CMake gui, or run the command
# line utility including the "-i" option.

set(CMAKE_LEGACY_CYGWIN_WIN32 0)
set (CMAKE_CXX_STANDARD 20)

project(xscript)

cmake_minimum_required(VERSION 2.8)
# Only for version 3 and up. cmake_policy(SET CMP0042 NEW)

set (VERSION 0.7.0)

if (WIN32)
	execute_process(COMMAND cmd /c echo %DATE% %TIME% OUTPUT_VARIABLE TIMESTAMP
			OUTPUT_STRIP_TRAILING_WHITESPACE)
else (WIN32)
	execute_process(COMMAND date "+%F %T%z" OUTPUT_VARIABLE TIMESTAMP
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	add_definitions (-D_DEBUG)
	SET(CMAKE_C_FLAGS "-O0 -Wall -g -ggdb")
	SET(CMAKE_CXX_FLAGS "-O0 -Wall -g -ggdb")

#	SET(CMAKE_C_FLAGS "-O3 -Wall")
#	SET(CMAKE_CXX_FLAGS "-O3 -Wall")
endif (WIN32)

add_definitions (-DCMAKE -DLGX_VERSION=\"${VERSION}\" -DLGX_TIMESTAMP=\"${TIMESTAMP}\")

if (WIN32)
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
	add_definitions("-D_CRT_NONSTDC_NO_DEPRECATE")
else (WIN32)

endif (WIN32)

# ========================================
# Include projects
# ========================================
add_subdirectory(src)