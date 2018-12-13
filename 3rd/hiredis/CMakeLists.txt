CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
INCLUDE(GNUInstallDirs)
PROJECT(redisearch)

# Get the version numbers
MACRO(getVersionBit name)

EXECUTE_PROCESS(
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMAND grep "${name}" ./3rd/hiredis/hiredis.h COMMAND awk "{print $3}"
    OUTPUT_VARIABLE "${name}"
    OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDMACRO(getVersionBit)

getVersionBit(HIREDIS_MAJOR)
getVersionBit(HIREDIS_MINOR)
getVersionBit(HIREDIS_PATCH)

MESSAGE("Detected version: ${HIREDIS_MAJOR}.${HIREDIS_MINOR}.${HIREDIS_PATCH}")

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

ADD_LIBRARY(hiredis STATIC
    async.c
    dict.c
    hiredis.c
    net.c
    read.c
    sds.c)

SET_TARGET_PROPERTIES(hiredis
    PROPERTIES
    VERSION "${HIREDIS_MAJOR}.${HIREDIS_MINOR}.${HIREDIS_PATCH}")
