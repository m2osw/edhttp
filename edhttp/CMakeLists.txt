# Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/edhttp
# contact@m2osw.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

AtomicNames("names.an")

project(edhttp)

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    http_client_server.cpp
    http_cookie.cpp
    http_date.cpp
    mime_type.cpp
    mkgmtime.c
    ${CMAKE_CURRENT_BINARY_DIR}/names.cpp
    quoted_printable.cpp
    string_part.cpp
    uri.cpp
    version.cpp
    weighted_http_string.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${SNAPDEV_INCLUDE_DIR}
        ${MAGIC_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_LIBRARIES}
        ${EVENTDISPATCHER_LIBRARIES}
        ${LIBEXCEPT_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${SNAPLOGGER_LIBRARIES}
        ${MAGIC_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION
        ${EDHTTP_VERSION_MAJOR}.${EDHTTP_VERSION_MINOR}

    SOVERSION
        ${EDHTTP_VERSION_MAJOR}
)

install(
    TARGETS
        ${PROJECT_NAME}

    LIBRARY DESTINATION
        lib
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/

    DESTINATION
        include/edhttp

    FILES_MATCHING PATTERN
        "*.h"
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/names.h

    DESTINATION
        include/edhttp
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/edhttp
)


# vim: ts=4 sw=4 et nocindent
