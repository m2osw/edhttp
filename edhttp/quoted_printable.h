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

// C
//
#include    <string>


namespace edhttp
{



int const QUOTED_PRINTABLE_FLAG_BINARY         = 0x0001;
int const QUOTED_PRINTABLE_FLAG_EDBIC          = 0x0002;
int const QUOTED_PRINTABLE_FLAG_LFONLY         = 0x0004; // many sendmail(1) do not like \r\n somehow
int const QUOTED_PRINTABLE_FLAG_NO_LONE_PERIOD = 0x0008;

std::string encode(std::string const & text, int flags = 0);
std::string decode(std::string const & text);



} // namespace edhttp
// vim: ts=4 sw=4 et
