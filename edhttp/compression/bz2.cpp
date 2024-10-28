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
#include    <cmath>


// C
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include    <bzlib.h>
#pragma GCC diagnostic pop


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



/** \brief Implementation of the BZip2 compressor (bz2).
 *
 * This class defines the bz2 compressor which compresses and decompresses
 * data using the bzip2 file format.
 *
 * \note
 * This implementation makes use of the libbz2 library to do all the
 * compression and decompression work.
 */
class bz2
    : public compressor
{
public:
                            bz2();

    virtual char const *    get_name() const override;
    virtual buffer_t        compress(buffer_t const & input, level_t level, bool text) override;
    virtual bool            compatible(buffer_t const & input) const override;
    virtual buffer_t        decompress(buffer_t const & input) override;
    virtual buffer_t        decompress(buffer_t const & input, std::size_t uncompressed_size) override;
};


bz2::bz2()
    : compressor("bz2")
{
}


char const * bz2::get_name() const
{
    return "bz2";
}


buffer_t bz2::compress(buffer_t const & input, level_t level, bool text)
{
    snapdev::NOT_USED(text);

    // clamp the level, just in case
    //
    level = std::clamp(level, static_cast<level_t>(0), static_cast<level_t>(100));

    // transform the 0 to 100 level to the block size of 1 to 9 in libbz2
    //
    int const block_size(std::clamp((level * 2 + 25) / 25, 1, 9));

    // compute output size largely so it always fit even if not compression
    // occurs: input buffer size + 1% of input buffer size + 600 bytes
    //
    std::size_t const size(static_cast<std::size_t>(ceil(input.size() * 1.01 + 600.0)));
    buffer_t result(size);
    unsigned int result_size(result.size());
    int const ret(BZ2_bzBuffToBuffCompress(
        reinterpret_cast<char *>(result.data()),
        &result_size,
        const_cast<char *>(reinterpret_cast<char const *>(input.data())),
        input.size(),
        block_size,
        0,      // no verbosity
        0));    // default work factor (30 at time of writing)
    if(ret != BZ_OK)
    {
        // compression failed, return input as is
        //
        return input;   // LCOV_EXCL_LINE
    }

    // lose the extra bytes if any
    //
    result.resize(result_size);

    return result;
}


bool bz2::compatible(buffer_t const & input) const
{
    // a file is at least 10 bytes
    // the magic code (identification) is 0x42 0x5A 0x68 (BZh)
    // then it must be followed by the size (work factor) 0x31 to 0x39
    //
    return input.size() >= 10
        && input[0] == 'B'
        && input[1] == 'Z'
        && input[2] == 'h'
        && input[3] >= '1' && input[3] <= '9';
}


buffer_t bz2::decompress(buffer_t const & input)
{
    // to decompress, we use the streaming version because we do not have
    // the size of the input anywhere; that way we can just grow the output
    // buffer each time we get some data from the decompressor

    // initialize the bz2 stream
    //
    buffer_t buffer(1024 * 100);
    bz_stream strm = {};
    strm.next_in = const_cast<char *>(reinterpret_cast<char const *>(input.data()));
    strm.avail_in = static_cast<unsigned int>(input.size());
    strm.next_out = reinterpret_cast<char *>(buffer.data());
    strm.avail_out = static_cast<unsigned int>(buffer.size());
    int ret(BZ2_bzDecompressInit(&strm, 0, 0));
    if(ret != BZ_OK)
    {
        return input; // LCOV_EXCL_LINE
    }
    struct cleanup
    {
        cleanup(bz_stream * strm)
            : f_stream(strm)
        {
        }

        cleanup(cleanup const & rhs) = delete;
        cleanup & operator = (cleanup const & rhs) = delete;

        ~cleanup()
        {
            BZ2_bzDecompressEnd(f_stream);
        }

        bz_stream * f_stream = nullptr;
    };
    cleanup defer(&strm);

    // prepare to call the inflate function (to do it in one go!)
    //
    buffer_t result;
    for(;;)
    {
        ret = BZ2_bzDecompress(&strm);
        if(ret != BZ_STREAM_END && ret != BZ_OK)
        {
            return input;
        }
        result.insert(result.end(), buffer.data(), buffer.data() + buffer.size() - strm.avail_out);
        if(ret == BZ_STREAM_END)
        {
            return result;
        }
        if(strm.avail_in == 0)
        {
            // we reached the end of the input but not the end of the
            // stream, so we've got a problem
            //
            return input;
        }
        strm.next_out = reinterpret_cast<char *>(buffer.data());
        strm.avail_out = static_cast<unsigned int>(buffer.size());
    }
}


buffer_t bz2::decompress(buffer_t const & input, std::size_t uncompressed_size)
{
    buffer_t result(uncompressed_size);
    unsigned int result_size(result.size());
    int const ret(BZ2_bzBuffToBuffDecompress(
        reinterpret_cast<char *>(result.data()),
        &result_size,
        const_cast<char *>(reinterpret_cast<char const *>(input.data())),
        input.size(),
        0,
        0));
    if(ret != BZ_OK)
    {
        return input;
    }

    return result;
}


// create a static definition of the bz2 compressor
//
bz2         g_bz2;



} // namespace edhttp
// vim: ts=4 sw=4 et
