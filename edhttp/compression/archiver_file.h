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
#pragma once

// self
//
#include    <edhttp/compression/compressor.h>



// snapdev
//
#include    <snapdev/timespec_ex.h>


// C++
//
#include    <span>



namespace edhttp
{



enum class file_type_t
{
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY
};


class archiver_file
{
public:
    void                    set_type(file_type_t type);
    void                    set_data(buffer_t const & data);
    void                    set_data(std::span<buffer_t::value_type const> const & data);
    void                    set_filename(std::string const & filename);
    void                    set_user(std::string const & user, uid_t uid);
    void                    set_group(std::string const & group, gid_t gid);
    void                    set_mode(mode_t mode);
    void                    set_mtime(snapdev::timespec_ex const & mtime);

    file_type_t             get_type() const;
    buffer_t const &        get_data() const;
    std::string const &     get_filename() const;
    std::string const &     get_user() const;
    std::string const &     get_group() const;
    uid_t                   get_uid() const;
    gid_t                   get_gid() const;
    mode_t                  get_mode() const;
    snapdev::timespec_ex const &
                            get_mtime() const;

private:
    file_type_t             f_type = file_type_t::FILE_TYPE_REGULAR;
    buffer_t                f_data = buffer_t();
    std::string             f_filename = std::string();
    std::string             f_user = std::string();
    std::string             f_group = std::string();
    uid_t                   f_uid = 0;
    gid_t                   f_gid = 0;
    mode_t                  f_mode = 0;
    snapdev::timespec_ex    f_mtime = snapdev::timespec_ex();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
