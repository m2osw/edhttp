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


#include <algorithm>
#include <ctime>


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



/** \brief Implementation of the GZip compressor (libz).
 *
 * This class defines the gzip compressor which compresses and decompresses
 * data using the gzip file format.
 *
 * \note
 * This implementation makes use of the zlib library to do all the
 * compression and decompression work.
 */
class gzip
    : public compressor
{
public:
                            gzip();

    virtual char const *    get_name() const override;
    virtual buffer_t        compress(buffer_t const & input, level_t level, bool text) override;
    virtual bool            compatible(buffer_t const & input) const override;
    virtual buffer_t        decompress(buffer_t const & input) override;
    virtual buffer_t        decompress(buffer_t const & input, std::size_t uncompressed_size) override;
};


gzip::gzip()
    : compressor("gzip")
{
}


char const * gzip::get_name() const
{
    return "gzip";
}


buffer_t gzip::compress(buffer_t const & input, level_t level, bool text)
{
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
    strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(input.data()));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    int ret(deflateInit2(&strm, zlib_level, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY));
#pragma GCC diagnostic pop
    if(ret != Z_OK)
    {
        // compression failed, return input as is
        //
        return input;   // LCOV_EXCL_LINE
    }

    // initialize the gzip header
    //
    gz_header header = {};
    header.text = text;
    header.time = time(nullptr);
    header.os = 3;
    header.comment = const_cast<Bytef *>(reinterpret_cast<Bytef const *>("Snap! Websites"));
    //header.hcrc = 1; -- would that be useful?
    ret = deflateSetHeader(&strm, &header);
    if(ret != Z_OK)
    {
        deflateEnd(&strm);      // LCOV_EXCL_LINE
        return input;           // LCOV_EXCL_LINE
    }

    // prepare to call the deflate function
    // (to do it in one go!)
    //
    // TODO check the size of the input buffer, if really large
    //      (256Kb?) then break this up in multiple iterations
    //
    buffer_t result;
    result.resize(static_cast<int>(deflateBound(&strm, strm.avail_in)));
    strm.avail_out = result.size();
    strm.next_out = reinterpret_cast<Bytef *>(result.data());

    // compress in one go
    //
    ret = deflate(&strm, Z_FINISH);
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


bool gzip::compatible(buffer_t const & input) const
{
    // the header is at least 10 bytes
    // the magic code (identification) is 0x1F 0x8B
    //
    return input.size() >= 10
        && input[0] == static_cast<buffer_t::value_type>(0x1F)
        && input[1] == static_cast<buffer_t::value_type>(0x8B);
}


buffer_t gzip::decompress(buffer_t const & input)
{
    // initialize the zlib stream
    //
    z_stream strm = {};

    // inflateInit2 expects the input to be defined
    //
    strm.avail_in = input.size();
    strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(input.data()));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    int ret(inflateInit2(&strm, 15 + 16));
#pragma GCC diagnostic pop
    if(ret != Z_OK)
    {
        // decompression failed, return input as is assuming it was not
        // compressed maybe...
        //
        return input; // LCOV_EXCL_LINE
    }

    // Unfortunately the zlib support for the gzip header does not help
    // us getting the ISIZE which is saved as the last 4 bytes of the
    // files (frankly?!)
    //
    // initialize the gzip header
    //gz_header header = {};
    //ret = inflateGetHeader(&strm, &header);
    //if(ret != Z_OK)
    //{
    //    inflateEnd(&strm);
    //    return input;
    //}

    // The size is saved in the last 4 bytes in little endian
    //
    if(strm.avail_in < 4)
    {
        // not enough data!?
        //
        inflateEnd(&strm);
        return input;
    }
    std::size_t const uncompressed_size(input[strm.avail_in - 4]
            | (input[strm.avail_in - 3] << 8)
            | (input[strm.avail_in - 2] << 16)
            | (input[strm.avail_in - 1] << 24));
    if(uncompressed_size >= 10UL * 1024UL * 1024UL * 1024UL)
    {
        // more than 10Gb!?
        //
        inflateEnd(&strm);
        return input;
    }
    if(uncompressed_size == 0)
    {
        inflateEnd(&strm);
        return buffer_t();
    }

    // prepare to call the inflate function (to do it in one go!)
    //
    buffer_t result;
    result.resize(static_cast<int>(uncompressed_size));
    strm.avail_out = result.size();
    strm.next_out = reinterpret_cast<Bytef *>(result.data());

    // decompress in one go
    //
    ret = inflate(&strm, Z_FINISH);
    if(ret != Z_STREAM_END)
    {
        inflateEnd(&strm);
        return input;
    }
    inflateEnd(&strm);
    return result;
}


buffer_t gzip::decompress(buffer_t const & input, std::size_t uncompressed_size)
{
    snapdev::NOT_USED(input, uncompressed_size);
    throw not_implemented("gzip::decompress() with a size is not implemented.");
}


// create a static definition of the gzip compressor
//
gzip        g_gzip;



} // namespace edhttp
// vim: ts=4 sw=4 et
