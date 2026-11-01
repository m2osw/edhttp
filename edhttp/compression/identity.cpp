// Copyright (c) 2013-2026  Made to Order Software Corp.  All Rights Reserved
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
#include    "edhttp/compression/compressor.h"

#include    "edhttp/exception.h"



// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>


// C++
//
#include    <algorithm>
#include    <ctime>


// C
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include    <zlib.h>
#pragma GCC diagnostic pop


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



class identity
    : public compressor
{
public:
                            identity();

    virtual char const *    get_name() const override;
    virtual buffer_t        compress(buffer_t const & input, level_t level, bool text) override;
    virtual bool            compatible(buffer_t const & input) const override;
    virtual buffer_t        decompress(buffer_t const & input) override;
    virtual buffer_t        decompress(buffer_t const & input, std::size_t uncompressed_size) override;
};


identity::identity()
    : compressor("identity")
{
}


char const * deflate::get_name() const
{
    return "identity";
}


buffer_t identity::compress(buffer_t const & input, level_t level, bool text)
{
    snapdev::NOT_USED(level, text);

    return input;
}


bool identity::compatible(buffer_t const & input) const
{
    snapdev::NOT_USED(input);

    // there is really no magic header in this one...
    //
    // although we could recognize files with a magic header which thus
    // are not compressed (i.e. JPEG, WAVE, MP3, XML, JSON etc.)
    //
    return false;
}


buffer_t identity::decompress(buffer_t const & input)
{
    return input;
}


buffer_t identity::decompress(buffer_t const & input, std::size_t uncompressed_size)
{
    return input;
}


// create a static definition of the deflate compressor
//
identity        g_identity;



} // namespace edhttp
// vim: ts=4 sw=4 et
