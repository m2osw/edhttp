# - Find EdHttp
#
# EDHTTP_FOUND        - System has EdHttp
# EDHTTP_INCLUDE_DIRS - The EdHttp include directories
# EDHTTP_LIBRARIES    - The libraries needed to use EdHttp
# EDHTTP_DEFINITIONS  - Compiler switches required for using EdHttp
#
# License:
#
# Copyright (c) 2011-2025  Made to Order Software Corp.  All Rights Reserved
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

find_path(
    EDHTTP_INCLUDE_DIR
        edhttp/version.h

    PATHS
        ENV EDHTTP_INCLUDE_DIR
)

find_library(
    EDHTTP_LIBRARY
        edhttp

    PATHS
        ${EDHTTP_LIBRARY_DIR}
        ENV EDHTTP_LIBRARY
)

mark_as_advanced(
    EDHTTP_INCLUDE_DIR
    EDHTTP_LIBRARY
)

set(EDHTTP_INCLUDE_DIRS ${EDHTTP_INCLUDE_DIR})
set(EDHTTP_LIBRARIES    ${EDHTTP_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    EdHttp
    REQUIRED_VARS
        EDHTTP_INCLUDE_DIR
        EDHTTP_LIBRARY
)

# vim: ts=4 sw=4 et
