# Copyright (c) 2023-2025  Made to Order Software Corp.  All Rights Reserved
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

##
## Tool to check the list of all the supported edhttp compressors
##
project(edhttp-list-compressors)

add_executable(${PROJECT_NAME}
    edhttp_list_compressors.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_INCLUDE_DIRS}
        ${SNAPDEV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    edhttp
    ${ADVGETOPT_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    DESTINATION
        bin
)


# vim: ts=4 sw=4 et
