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
#include    <snapdev/raii_generic_deleter.h>


// C++
//
#include    <algorithm>
#include    <iomanip>


// C
//
#include    <lzma.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



typedef std::unique_ptr<lzma_stream *, snapdev::raii_generic_deleter<lzma_stream *, nullptr, decltype(&::lzma_end), &::lzma_end>> raii_lzma_t;


/** \brief Implementation of the XZ compressor (lzma).
 *
 * This class defines the xz compressor which compresses and decompresses
 * data using the xz file format.
 *
 * \note
 * This implementation makes use of the lzma library to do all the
 * compression and decompression work.
 */
class xz
    : public compressor
{
public:
                            xz();

    virtual char const *    get_name() const override;
    virtual buffer_t        compress(buffer_t const & input, level_t level, bool text) override;
    virtual bool            compatible(buffer_t const & input) const override;
    virtual buffer_t        decompress(buffer_t const & input) override;
    virtual buffer_t        decompress(buffer_t const & input, std::size_t uncompressed_size) override;
};


xz::xz()
    : compressor("xz")
{
}


char const * xz::get_name() const
{
    return "xz";
}


buffer_t xz::compress(buffer_t const & input, level_t level, bool text)
{
    snapdev::NOT_USED(text);

    // clamp the level, just in case
    //
    level = std::clamp(level, static_cast<level_t>(0), static_cast<level_t>(100));

    // transform the 0 to 100 level to the standard 0 to 9 in xz
    //
    int const xz_level(std::clamp((level * 8 + 10) / 90, 0, 9));

    lzma_stream strm = LZMA_STREAM_INIT;
    raii_lzma_t defer(&strm);

    lzma_ret ret(lzma_easy_encoder(&strm, xz_level, LZMA_CHECK_CRC64));
    if(ret != LZMA_OK)
    {
        return input; // LCOV_EXCL_LINE
    }

    strm.avail_in = input.size();
    strm.next_in = input.data();

    // we don't want results that are larger than the input buffer so we
    // can just use a result of that size to start with a resize to a
    // smaller buffer if the compression worked as expected
    //
    buffer_t result;

    std::uint8_t buf[4 * 1024];
    do
    {
        strm.avail_out = sizeof(buf);
        strm.next_out = buf;

        ret = lzma_code(&strm, LZMA_FINISH);
        if(ret != LZMA_OK
        && ret != LZMA_STREAM_END)
        {
            // we could not compress that buffer?!
            //
            // (there is a total size limit of 2^63, but that would
            // require too much memory for `input` so really unlikely)
            //
            return input; // LCOV_EXCL_LINE
        }
        result.insert(result.end(), buf, buf + sizeof(buf) - strm.avail_out);
    }
    while(ret != LZMA_STREAM_END);

    return result;
}


bool xz::compatible(buffer_t const & input) const
{
    // the header is at least 10 bytes
    // the magic code (identification) is 0x1F 0x8B
    //
    return input.size() >= 10
        && input[0] == static_cast<buffer_t::value_type>(0xFD)
        && input[1] == static_cast<buffer_t::value_type>(0x37)  // 7
        && input[2] == static_cast<buffer_t::value_type>(0x7A)  // z
        && input[3] == static_cast<buffer_t::value_type>(0x58)  // X
        && input[4] == static_cast<buffer_t::value_type>(0x5A); // Z
}


buffer_t xz::decompress(buffer_t const & input)
{
    // initialize the xz stream
    //
    lzma_stream strm = LZMA_STREAM_INIT;
    raii_lzma_t defer(&strm);

    lzma_ret ret(lzma_auto_decoder(&strm, UINT64_MAX, 0));
    if(ret != LZMA_OK)
    {
        return input; // LCOV_EXCL_LINE
    }

    // TBD: is there a way to know about the size?
    //
    buffer_t result;

    strm.avail_in = input.size();
    strm.next_in = input.data();

    std::uint8_t buf[1024 * 4];

    do
    {
        strm.avail_out = sizeof(buf);
        strm.next_out = buf;

        ret = lzma_code(&strm, LZMA_FINISH);
        if(ret != LZMA_STREAM_END
        && ret != LZMA_OK)
        {
            return input;
        }

        result.insert(result.end(), buf, buf + sizeof(buf) - strm.avail_out);
    }
    while(ret != LZMA_STREAM_END);

    return result;
}


buffer_t xz::decompress(buffer_t const & input, std::size_t uncompressed_size)
{
    snapdev::NOT_USED(input, uncompressed_size);
    throw not_implemented("xz::decompress() with a size is not implemented.");
}


// create a static definition of the xz compressor
//
xz          g_xz;



} // namespace edhttp
// vim: ts=4 sw=4 et
