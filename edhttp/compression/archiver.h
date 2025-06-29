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
#include    <edhttp/compression/archiver_archive.h>
#include    <edhttp/compression/archiver_file.h>



namespace edhttp
{



class archiver
{
public:
                            archiver(char const * name);
    virtual                 ~archiver();

    virtual char const *    get_name() const = 0;

    virtual void            append_file(archiver_archive & archive, archiver_file const & file) = 0;
    virtual bool            next_file(archiver_archive & archive, archiver_file & file) const = 0;
    virtual void            rewind(archiver_archive & archive) = 0;
};


advgetopt::string_list_t    archiver_list();
archiver *                  get_archiver(std::string const & archiver_name);



} // namespace edhttp
// vim: ts=4 sw=4 et
