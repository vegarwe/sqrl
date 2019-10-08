cmake_minimum_required(VERSION 3.0.0)

execute_process(COMMAND git describe --tags
    WORKING_DIRECTORY ${local_source_dir}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip
# file from Github instead of a checkout)
if ("${GIT_VERSION}" STREQUAL "")
    set(GIT_VERSION "N/A")
endif()

set(VERSION_FILE "gitversion.h")
set(VERSION "// Generate ${VERSION_FILE}
\#pragma once
\#define GIT_VERSION \"${GIT_VERSION}\"
")


if(EXISTS ${local_binary_dir}/${VERSION_FILE})
    file(READ ${local_binary_dir}/${VERSION_FILE} VERSION_)
else()
    set(VERSION_ "")
endif()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
    file(WRITE ${local_binary_dir}/${VERSION_FILE} "${VERSION}")
endif()
