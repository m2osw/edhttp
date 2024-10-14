// Copyright (c) 2013-2024  Made to Order Software Corp.  All Rights Reserved
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
//#include    "snapwebsites/snap_string_list.h"



// advgetopt
//
#include    <advgetopt/utils.h>


// C++
//
//#include    <string>
//#include    <vector>


// C
//
//#include <unistd.h>



namespace edhttp
{



// compression level is a percent (a number from 0 to 100)
typedef std::uint8_t                    level_t;


// a buffer is an array of bytes
typedef std::vector<std::uint8_t>       buffer_t;


// all compressors derive from this interface
class compressor
{
public:
    static char const * BEST_COMPRESSION;
    static char const * NO_COMPRESSION;

                        compressor(char const * name);
    virtual             ~compressor();

    virtual char const *get_name() const = 0;
    virtual char const *get_tags() const = 0;
    virtual buffer_t    compress(buffer_t const & input, level_t level, bool text) = 0;
    virtual bool        compatible(buffer_t const & input) const = 0;
    virtual buffer_t    decompress(buffer_t const & input) = 0;
    virtual buffer_t    decompress(buffer_t const & input, size_t uncompressed_size) = 0;
};


advgetopt::string_list_t    compressor_list();
compressor *                get_compressor(std::string const & compressor_name);
buffer_t                    compress(std::string & compressor_name, buffer_t const & input, level_t level, bool text = false);
buffer_t                    decompress(std::string & compressor_name, buffer_t const & input);



} // namespace edhttp
// vim: ts=4 sw=4 et
