// Copyright (c) 2011-2025  Made to Order Software Corp.  All Rights Reserved
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

// C++
//
#include    <string>



namespace edhttp
{



enum class date_format_t
{
    DATE_FORMAT_SHORT,
    DATE_FORMAT_SHORT_US,
    DATE_FORMAT_LONG,
    DATE_FORMAT_TIME,
    DATE_FORMAT_EMAIL,
    DATE_FORMAT_HTTP
};


std::string     date_to_string(time_t v, date_format_t date_format);
time_t          string_to_date(std::string const & date);
int             last_day_of_month(int month, int year);



} // namespace edhttp
// vim: ts=4 sw=4 et

