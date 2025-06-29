// Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "edhttp/compression/archiver_archive.h"



// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



buffer_t & archiver_archive::get()
{
    return f_archive;
}


buffer_t const & archiver_archive::get() const
{
    return f_archive;
}


void archiver_archive::set(buffer_t const & input)
{
    f_archive = input;
}


std::size_t archiver_archive::get_pos() const
{
    return f_pos;
}


void archiver_archive::set_pos(std::size_t pos)
{
    f_pos = pos;
}


void archiver_archive::advance_pos(std::size_t offset)
{
    f_pos += offset;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
