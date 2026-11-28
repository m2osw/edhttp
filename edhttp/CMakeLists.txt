# Copyright (c) 2011-2026  Made to Order Software Corp.  All Rights Reserved
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

find_package(PkgConfig REQUIRED)
pkg_check_modules(NGHTTP2 REQUIRED IMPORTED_TARGET libnghttp2)

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    cache_control.cpp
    health.cpp
    http_cache.cpp
    http_client_connection.cpp
    http_cookie.cpp
    http_date.cpp
    http_link.cpp
    http_request.cpp
    http_response.cpp
    http_sync_client.cpp
    mime_type.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/names.cpp
    quoted_printable.cpp
    token.cpp
    uri.cpp
    utils.cpp
    validator_uri.cpp
    version.cpp
    parametrized_value.cpp
    parametrized_value_list.cpp

    compression/archiver.cpp
    compression/archiver_archive.cpp
    compression/archiver_file.cpp
    compression/bz2.cpp
    compression/compressor.cpp
    compression/deflate.cpp
    compression/gzip.cpp
    compression/tar.cpp
    compression/xz.cpp

    http/http_client_connection_impl.cpp
    http/http_domain.cpp
    http/http_tcp_client_async_connection.cpp
    http/http_tcp_client_connection.cpp
    http/http_udp_connection.cpp
    # http/http_udp_responsive_connection.cpp -- not too sure why I thought I'd need this one?
    http/runner.cpp
    http/thread_signal_handler.cpp
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        ZLIB_CONST
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_INCLUDE_DIRS}
        ${EVENTDISPATCHER_INCLUDE_DIRS}
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${LIBTLD_INCLUDE_DIRS}
        ${MAGIC_INCLUDE_DIRS}
        ${MAGPIE_INCLUDE_DIRS}
        ${SNAPDEV_INCLUDE_DIRS}
        ${SNAPLOGGER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_LIBRARIES}
        ${EVENTDISPATCHER_LIBRARIES}
        ${LIBEXCEPT_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${MAGIC_LIBRARIES}
        ${MAGPIE_LIBRARIES}
        ${SNAPLOGGER_LIBRARIES}
        PkgConfig::NGHTTP2
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
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/compression/

    DESTINATION
        include/edhttp/compression

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
