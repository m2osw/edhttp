// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/edhttp
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

// advgetopt
//
#include    <advgetopt/advgetopt.h>



namespace edhttp
{



// the key for the health status
//
constexpr char const        DIAG_KEY_HEALTH[] = "HEALTH";


// some statuses
//
constexpr char const        HEALTH_STARTING[] = "STARTING";
constexpr char const        HEALTH_OK[]       = "OK";
constexpr char const        HEALTH_ERROR[]    = "ERROR";
constexpr char const        HEALTH_FAILED[]   = "FAILED";


void            add_health_options(advgetopt::getopt & opts);
bool            process_health_options(advgetopt::getopt & opts);
void            set_status(std::string const & status);
std::string     get_status();



} // namespace edhttp
// vim: ts=4 sw=4 et
