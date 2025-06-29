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
#include    "edhttp/compression/archiver_file.h"



// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



void archiver_file::set_type(file_type_t type)
{
    f_type = type;
}


void archiver_file::set_data(buffer_t const & data)
{
    f_data = data;
}


void archiver_file::set_data(std::span<buffer_t::value_type const> const & data)
{
    f_data.assign(data.data(), data.data() + data.size());
}


void archiver_file::set_filename(std::string const & filename)
{
    f_filename = filename;
}


void archiver_file::set_user(std::string const & user, uid_t uid)
{
    f_user = user;
    f_uid = uid;
}


void archiver_file::set_group(std::string const & group, gid_t gid)
{
    f_group = group;
    f_gid = gid;
}


void archiver_file::set_mode(mode_t mode)
{
    f_mode = mode;
}


void archiver_file::set_mtime(snapdev::timespec_ex const & mtime)
{
    f_mtime = mtime;
}


file_type_t archiver_file::get_type() const
{
    return f_type;
}


buffer_t const & archiver_file::get_data() const
{
    return f_data;
}


std::string const & archiver_file::get_filename() const
{
    return f_filename;
}


std::string const & archiver_file::get_user() const
{
    return f_user;
}


std::string const & archiver_file::get_group() const
{
    return f_group;
}


uid_t archiver_file::get_uid() const
{
    return f_uid;
}


gid_t archiver_file::get_gid() const
{
    return f_gid;
}


mode_t archiver_file::get_mode() const
{
    return f_mode;
}


snapdev::timespec_ex const & archiver_file::get_mtime() const
{
    return f_mtime;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
