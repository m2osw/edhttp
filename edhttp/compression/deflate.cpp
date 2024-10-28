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



class deflate
    : public compressor
{
public:
                            deflate();

    virtual char const *    get_name() const override;
    virtual buffer_t        compress(buffer_t const & input, level_t level, bool text) override;
    virtual bool            compatible(buffer_t const & input) const override;
    virtual buffer_t        decompress(buffer_t const & input) override;
    virtual buffer_t        decompress(buffer_t const & input, std::size_t uncompressed_size) override;
};


deflate::deflate()
    : compressor("deflate")
{
}


char const * deflate::get_name() const
{
    return "deflate";
}


buffer_t deflate::compress(buffer_t const & input, level_t level, bool text)
{
    snapdev::NOT_USED(text);

    // clamp the level, just in case
    //
    level = std::clamp(level, static_cast<level_t>(0), static_cast<level_t>(100));

    // transform the 0 to 100 level to the standard 1 to 9 in zlib
    //
    int const zlib_level(std::clamp((level * 2 + 25) / 25, Z_BEST_SPEED, Z_BEST_COMPRESSION));

    // initialize the zlib stream
    //
    z_stream strm = {};

    // deflateInit2 expects the input to be defined
    //
    strm.avail_in = input.size();
    strm.next_in = const_cast<Bytef *>(reinterpret_cast<Bytef const *>(input.data()));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    int ret(deflateInit2(&strm, zlib_level, Z_DEFLATED, 15, 9, Z_DEFAULT_STRATEGY));
#pragma GCC diagnostic pop
    if(ret != Z_OK)
    {
        // compression failed, return input as is
        //
        return input;           // LCOV_EXCL_LINE
    }

    // prepare to call the deflate function
    // (to do it in one go!)
    //
    // TODO check the size of the input buffer, if really large
    //      (256Kb?) then break this up in multiple iterations
    //
    buffer_t result(deflateBound(&strm, strm.avail_in));
    strm.avail_out = result.size();
    strm.next_out = reinterpret_cast<Bytef *>(result.data());

    // compress in one go
    //
    ret = ::deflate(&strm, Z_FINISH);
    if(ret != Z_STREAM_END)
    {
        deflateEnd(&strm);      // LCOV_EXCL_LINE
        return input;           // LCOV_EXCL_LINE
    }

    // lose the extra size returned by deflateBound()
    //
    result.resize(result.size() - strm.avail_out);
    deflateEnd(&strm);
    return result;
}


bool deflate::compatible(buffer_t const & input) const
{
    snapdev::NOT_USED(input);

    // there is no magic header in this one...
    //
    return false;
}


buffer_t deflate::decompress(buffer_t const & input)
{
    // the decompress function for "deflate" requires the size in
    // our case so this function is not implemented for now...
    //
    snapdev::NOT_USED(input);
    throw not_implemented("deflate::decompress() without the uncompressed_size parameter is not implemented.");
}


buffer_t deflate::decompress(buffer_t const & input, std::size_t uncompressed_size)
{
    // by default we cannot reach this function, if we get called, then
    // the caller specifically wanted to call us, in such a case we
    // expect the size of the uncompressed data to be specified...

    // if the output is an empty buffer, then we need to return an empty buffer
    //
    if(uncompressed_size == 0)
    {
        return buffer_t();
    }

    // initialize the zlib stream
    //
    z_stream strm = {};

    // inflateInit expects the input to be defined
    //
    strm.avail_in = input.size();
    strm.next_in = reinterpret_cast<Bytef const *>(input.data());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    int ret(inflateInit(&strm));
#pragma GCC diagnostic pop
    if(ret != Z_OK)
    {
        // decompression failed, return input as is
        //
        return input; // LCOV_EXCL_LINE
    }

    // prepare to call the inflate function
    // (to do it in one go!)
    //
    buffer_t result(uncompressed_size);
    strm.avail_out = result.size();
    strm.next_out = reinterpret_cast<Bytef *>(result.data());

    // decompress in one go
    //
    ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
    if(ret != Z_STREAM_END)
    {
        // decompression failed, return input as is
        //
        return input;
    }

    return result;
}


// create a static definition of the deflate compressor
//
deflate         g_deflate;



} // namespace edhttp
// vim: ts=4 sw=4 et
