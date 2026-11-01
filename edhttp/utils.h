// Copyright (c) 2011-2026  Made to Order Software Corp.  All Rights Reserved
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

// magpie
//
#include    <magpie/options.h>


// C++
//
#include    <cstdint>
#include    <string>
#include    <vector>



namespace edhttp
{



typedef std::vector<std::uint8_t>       buffer_t;


void                    accept_h3(std::string const & domain, magpie::duration_t ttl);
bool                    accept_h3(std::string const & domain);



} // namespace edhttp
// vim: ts=4 sw=4 et
