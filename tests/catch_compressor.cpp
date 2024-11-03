// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/edhttp
// contact@m2osw.com
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

/** \file
 * \brief Verify the snap_uri class.
 *
 * This file implements tests to verify that the snap_uri
 * class functions as expected.
 */

// edhttp
//
#include    <edhttp/compression/compressor.h>

#include    <edhttp/exception.h>
#include    <edhttp/token.h>


// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/file_contents.h>
#include    <snapdev/not_used.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <random>


// last include
//
#include    <snapdev/poison.h>



namespace
{



class compressor_named
    : public edhttp::compressor
{
public:
                                compressor_named(char const * name) : compressor(name) {}

    virtual char const *        get_name() const override { return nullptr; }
    virtual edhttp::buffer_t    compress(edhttp::buffer_t const & input, edhttp::level_t level, bool text) override { snapdev::NOT_USED(input, level, text); return edhttp::buffer_t(); }
    virtual bool                compatible(edhttp::buffer_t const & input) const override { snapdev::NOT_USED(input); return false; }
    virtual edhttp::buffer_t    decompress(edhttp::buffer_t const & input) override { snapdev::NOT_USED(input); return edhttp::buffer_t(); }
    virtual edhttp::buffer_t    decompress(edhttp::buffer_t const & input, std::size_t uncompressed_size) override { snapdev::NOT_USED(input, uncompressed_size); return edhttp::buffer_t(); }
};



} // no name namespace



CATCH_TEST_CASE("compressor_bz2", "[compression]")
{
    CATCH_START_SECTION("compressor_bz2: verify bz2 compressor")
    {
        edhttp::compressor * bz2(edhttp::get_compressor("bz2"));
        CATCH_REQUIRE(bz2 != nullptr);
        CATCH_REQUIRE(strcmp(bz2->get_name(), "bz2") == 0);

        // generate a random buffer to compress of 1kb to 16kb in size
        //
        auto const input(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024 * 16));
        for(edhttp::level_t level(0); level <= 100; level += 10)
        {
            edhttp::buffer_t const compressed(bz2->compress(input, level, rand() & 1 == 0));

            bool equal(true);
            if(compressed.size() == input.size())
            {
                for(std::size_t pos(0); pos < compressed.size(); ++pos)
                {
                    if(compressed[pos] != input[pos])
                    {
                        equal = false;
                        break;
                    }
                }
            }
            else
            {
                equal = false;
            }
            CATCH_REQUIRE_FALSE(equal);

            edhttp::buffer_t const decompressed(bz2->decompress(compressed));
            CATCH_REQUIRE(decompressed.size() == input.size());

            equal = true;
            for(std::size_t pos(0); pos < decompressed.size(); ++pos)
            {
                if(decompressed[pos] != input[pos])
                {
                    equal = false;
                    break;
                }
            }
            CATCH_REQUIRE(equal);

            for(std::size_t s(2); s < 9; ++s)
            {
                edhttp::buffer_t const broken_compressed_small(compressed.data(), compressed.data() + s);
                edhttp::buffer_t const compressed_repeat(bz2->decompress(broken_compressed_small));
                CATCH_REQUIRE(compressed_repeat == broken_compressed_small);

                edhttp::buffer_t broken_compressed_small2(compressed.data(), compressed.data() + s);
                broken_compressed_small2.back() ^= 0xff;
                edhttp::buffer_t const compressed_repeat2(bz2->decompress(broken_compressed_small2));
                CATCH_REQUIRE(compressed_repeat2 == broken_compressed_small2);

                edhttp::buffer_t const compressed_repeat3(bz2->decompress(broken_compressed_small2, input.size()));
                CATCH_REQUIRE(compressed_repeat3 == broken_compressed_small2);
            }

            // we do recognize a bz2 buffer
            //
            CATCH_REQUIRE_FALSE(bz2->compatible(input));
            CATCH_REQUIRE(bz2->compatible(compressed));
            CATCH_REQUIRE_FALSE(bz2->compatible(decompressed));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_bz2: verify small buffers with bz2 compressor")
    {
        edhttp::compressor * bz2(edhttp::get_compressor("bz2"));
        CATCH_REQUIRE(bz2 != nullptr);

        // generate a random buffer to compress of 1kb to 16kb in size
        //
        for(int size(1); size < 20; ++size)
        {
            auto const input(SNAP_CATCH2_NAMESPACE::random_buffer(size, size));
            edhttp::buffer_t const compressed(bz2->compress(input, rand() % 95 + 5, rand() & 1 == 0));

            bool equal(true);
            if(compressed.size() == input.size())
            {
                for(std::size_t pos(0); pos < compressed.size(); ++pos)
                {
                    if(compressed[pos] != input[pos])
                    {
                        equal = false;
                        break;
                    }
                }
            }
            else
            {
                equal = false;
            }
            CATCH_REQUIRE_FALSE(equal);

            edhttp::buffer_t const decompressed(bz2->decompress(compressed));
            CATCH_REQUIRE(decompressed.size() == input.size());

            equal = true;
            for(std::size_t pos(0); pos < decompressed.size(); ++pos)
            {
                if(decompressed[pos] != input[pos])
                {
                    equal = false;
                    break;
                }
            }
            CATCH_REQUIRE(equal);

            // we do recognize a bz2 buffer
            //
            CATCH_REQUIRE(bz2->compatible(compressed));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_bz2: verify invalid bz2 magic length")
    {
        edhttp::compressor * bz2(edhttp::get_compressor("bz2"));
        CATCH_REQUIRE(bz2 != nullptr);
        CATCH_REQUIRE(strcmp(bz2->get_name(), "bz2") == 0);

        for(std::size_t size(0); size < 9; ++size)
        {
            // generate a random buffer to compress of 1kb to 16kb in size
            //
            auto input(SNAP_CATCH2_NAMESPACE::random_buffer(size, size));
            if(size >= 1)
            {
                input[0] = 'B';
            }
            if(size >= 2)
            {
                input[1] = 'Z';
            }
            if(size >= 3)
            {
                input[2] = 'h';
            }
            if(size >= 4)
            {
                input[3] = rand() % 10 + '0';
            }
            CATCH_REQUIRE_FALSE(bz2->compatible(input));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_bz2: attempt bz2 compressing an empty buffer")
    {
        edhttp::compressor * bz2(edhttp::get_compressor("bz2"));
        CATCH_REQUIRE(bz2 != nullptr);
        CATCH_REQUIRE(strcmp(bz2->get_name(), "bz2") == 0);

        edhttp::buffer_t const empty;
        CATCH_REQUIRE_FALSE(bz2->compatible(empty));

        for(edhttp::level_t level(0); level <= 120; level += rand() % 10 + 1)
        {
            edhttp::buffer_t const compressed(bz2->compress(empty, level, false));

            // an empty buffer can be "compressed" (the output is bigger, but it
            // does not fail)
            //
            //CATCH_REQUIRE(empty == compressed);

            edhttp::buffer_t const decompressed(bz2->decompress(compressed));
            CATCH_REQUIRE(empty == decompressed);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("compressor_deflate", "[compression]")
{
    CATCH_START_SECTION("compressor_deflate: verify deflate compressor")
    {
        edhttp::compressor * deflate(edhttp::get_compressor("deflate"));
        CATCH_REQUIRE(deflate != nullptr);
        CATCH_REQUIRE(strcmp(deflate->get_name(), "deflate") == 0);

        // generate a random buffer to compress of 1kb to 16kb in size
        //
        auto const input(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024 * 16));
        for(edhttp::level_t level(0); level <= 100; level += 10)
        {
            edhttp::buffer_t const compressed(deflate->compress(input, level, rand() & 1 == 0));

            bool equal(true);
            if(compressed.size() == input.size())
            {
                for(std::size_t pos(0); pos < compressed.size(); ++pos)
                {
                    if(compressed[pos] != input[pos])
                    {
                        equal = false;
                        break;
                    }
                }
            }
            else
            {
                equal = false;
            }
            CATCH_REQUIRE_FALSE(equal);

            edhttp::buffer_t const decompressed(deflate->decompress(compressed, input.size()));
            CATCH_REQUIRE(decompressed.size() == input.size());

            equal = true;
            for(std::size_t pos(0); pos < decompressed.size(); ++pos)
            {
                if(decompressed[pos] != input[pos])
                {
                    equal = false;
                    break;
                }
            }
            CATCH_REQUIRE(equal);

            // decompress with the wrong size has to fail
            // failure in this case means the input buffer is returned as is
            //
            edhttp::buffer_t compressed_repeat(deflate->decompress(compressed, input.size() / 2));
            CATCH_REQUIRE(compressed_repeat.size() == compressed.size());
            CATCH_REQUIRE(compressed_repeat == compressed);

            compressed_repeat = deflate->decompress(compressed, 3);
            CATCH_REQUIRE(compressed_repeat.size() == compressed.size());
            CATCH_REQUIRE(compressed_repeat == compressed);

            // we cannot recognize a deflated buffer because it has no magic
            //
            CATCH_REQUIRE_FALSE(deflate->compatible(input));
            CATCH_REQUIRE_FALSE(deflate->compatible(compressed));
            CATCH_REQUIRE_FALSE(deflate->compatible(decompressed));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_deflate: attempt deflate compressing an empty buffer")
    {
        edhttp::compressor * deflate(edhttp::get_compressor("deflate"));
        CATCH_REQUIRE(deflate != nullptr);
        CATCH_REQUIRE(strcmp(deflate->get_name(), "deflate") == 0);

        edhttp::buffer_t const empty;
        CATCH_REQUIRE_FALSE(deflate->compatible(empty));

        for(edhttp::level_t level(rand() % 6); level <= 120; level += rand() % 10 + 1)
        {
            edhttp::buffer_t const compressed(deflate->compress(empty, level, false));

            // an empty buffer can be "compressed" (the output is bigger, but it
            // does not fail)
            //
            //CATCH_REQUIRE(empty == compressed);

            edhttp::buffer_t const decompressed(deflate->decompress(compressed, 0));
            CATCH_REQUIRE(empty == decompressed);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_deflate: compress small buffers with deflate")
    {
        edhttp::compressor * deflate(edhttp::get_compressor("deflate"));
        CATCH_REQUIRE(deflate != nullptr);
        CATCH_REQUIRE(strcmp(deflate->get_name(), "deflate") == 0);

        for(std::size_t size(1); size < 1024; ++size)
        {
            auto const input(SNAP_CATCH2_NAMESPACE::random_buffer(size, size));
            edhttp::buffer_t const compressed(deflate->compress(input, rand() % 95 + 5, (rand() & 1) == 0));

            edhttp::buffer_t const decompressed(deflate->decompress(compressed, size));
            CATCH_REQUIRE(input == decompressed);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("compressor_gzip", "[compression]")
{
    CATCH_START_SECTION("compressor_gzip: verify gzip compressor")
    {
        edhttp::compressor * gzip(edhttp::get_compressor("gzip"));
        CATCH_REQUIRE(gzip != nullptr);
        CATCH_REQUIRE(strcmp(gzip->get_name(), "gzip") == 0);

        // generate a random buffer to compress of 1kb to 16kb in size
        //
        auto const input(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024 * 16));
        for(edhttp::level_t level(0); level <= 100; level += 10)
        {
            edhttp::buffer_t const compressed(gzip->compress(input, level, rand() & 1 == 0));

            bool equal(true);
            if(compressed.size() == input.size())
            {
                for(std::size_t pos(0); pos < compressed.size(); ++pos)
                {
                    if(compressed[pos] != input[pos])
                    {
                        equal = false;
                        break;
                    }
                }
            }
            else
            {
                equal = false;
            }
            CATCH_REQUIRE_FALSE(equal);

            edhttp::buffer_t const decompressed(gzip->decompress(compressed));
            CATCH_REQUIRE(decompressed.size() == input.size());

            equal = true;
            for(std::size_t pos(0); pos < decompressed.size(); ++pos)
            {
                if(decompressed[pos] != input[pos])
                {
                    equal = false;
                    break;
                }
            }
            CATCH_REQUIRE(equal);

            for(std::size_t s(2); s < 9; ++s)
            {
                edhttp::buffer_t const broken_compressed_small(compressed.data(), compressed.data() + s);
                edhttp::buffer_t const compressed_repeat(gzip->decompress(broken_compressed_small));
                CATCH_REQUIRE(compressed_repeat == broken_compressed_small);
            }

            // we do recognize a gzip buffer
            //
            CATCH_REQUIRE_FALSE(gzip->compatible(input));
            CATCH_REQUIRE(gzip->compatible(compressed));
            CATCH_REQUIRE_FALSE(gzip->compatible(decompressed));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_gzip: verify small buffers with gzip compressor")
    {
        edhttp::compressor * gzip(edhttp::get_compressor("gzip"));
        CATCH_REQUIRE(gzip != nullptr);

        // generate a random buffer to compress of 1kb to 16kb in size
        //
        for(int size(1); size < 20; ++size)
        {
            auto const input(SNAP_CATCH2_NAMESPACE::random_buffer(size, size));
            edhttp::buffer_t const compressed(gzip->compress(input, rand() % 95 + 5, rand() & 1 == 0));

            bool equal(true);
            if(compressed.size() == input.size())
            {
                for(std::size_t pos(0); pos < compressed.size(); ++pos)
                {
                    if(compressed[pos] != input[pos])
                    {
                        equal = false;
                        break;
                    }
                }
            }
            else
            {
                equal = false;
            }
            CATCH_REQUIRE_FALSE(equal);

            edhttp::buffer_t const decompressed(gzip->decompress(compressed));
            CATCH_REQUIRE(decompressed.size() == input.size());

            equal = true;
            for(std::size_t pos(0); pos < decompressed.size(); ++pos)
            {
                if(decompressed[pos] != input[pos])
                {
                    equal = false;
                    break;
                }
            }
            CATCH_REQUIRE(equal);

            // we do recognize a gzip buffer
            //
            CATCH_REQUIRE(gzip->compatible(compressed));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_gzip: verify invalid gzip magic length")
    {
        edhttp::compressor * gzip(edhttp::get_compressor("gzip"));
        CATCH_REQUIRE(gzip != nullptr);
        CATCH_REQUIRE(strcmp(gzip->get_name(), "gzip") == 0);

        for(std::size_t size(0); size < 10; ++size)
        {
            // generate a random buffer to compress of 1kb to 16kb in size
            //
            auto input(SNAP_CATCH2_NAMESPACE::random_buffer(size, size));
            if(size >= 1)
            {
                input[0] = 0x1F;
            }
            if(size >= 2)
            {
                input[1] = 0x8B;
            }
            CATCH_REQUIRE_FALSE(gzip->compatible(input));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_gzip: attempt gzip compressing an empty buffer")
    {
        edhttp::compressor * gzip(edhttp::get_compressor("gzip"));
        CATCH_REQUIRE(gzip != nullptr);
        CATCH_REQUIRE(strcmp(gzip->get_name(), "gzip") == 0);

        edhttp::buffer_t const empty;
        CATCH_REQUIRE_FALSE(gzip->compatible(empty));

        for(edhttp::level_t level(0); level <= 120; level += rand() % 10 + 1)
        {
            edhttp::buffer_t const compressed(gzip->compress(empty, level, false));

            // an empty buffer can be "compressed" (but the output is bigger,
            // yet it does not fail with gzip)
            //
            //CATCH_REQUIRE(empty == compressed);

            edhttp::buffer_t const decompressed(gzip->decompress(compressed));
            CATCH_REQUIRE(empty == decompressed);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("compressor_xz", "[compression]")
{
    CATCH_START_SECTION("compressor_xz: verify xz compressor")
    {
        // use a text file (this very file) because that compresses well
        //
        snapdev::file_contents source(SNAP_CATCH2_NAMESPACE::g_source_dir() + "/tests/catch_compressor.cpp");
        CATCH_REQUIRE(source.read_all());
        std::string const data(source.contents());
        edhttp::buffer_t const input(data.begin(), data.end());

        edhttp::compressor * xz(edhttp::get_compressor("xz"));
        CATCH_REQUIRE(xz != nullptr);
        CATCH_REQUIRE(strcmp(xz->get_name(), "xz") == 0);

        for(edhttp::level_t level(0); level <= 100; level += 10)
        {
            edhttp::buffer_t const compressed(xz->compress(input, level, rand() & 1 == 0));

            bool equal(true);
            if(compressed.size() == input.size())
            {
                for(std::size_t pos(0); pos < compressed.size(); ++pos)
                {
                    if(compressed[pos] != input[pos])
                    {
                        equal = false;
                        break;
                    }
                }
            }
            else
            {
                equal = false;
            }
            CATCH_REQUIRE_FALSE(equal);

            edhttp::buffer_t const decompressed(xz->decompress(compressed));
            CATCH_REQUIRE(decompressed.size() == input.size());

            equal = true;
            for(std::size_t pos(0); pos < decompressed.size(); ++pos)
            {
                if(decompressed[pos] != input[pos])
                {
                    equal = false;
                    break;
                }
            }
            CATCH_REQUIRE(equal);

            for(std::size_t s(2); s < 9; ++s)
            {
                edhttp::buffer_t const broken_compressed_small(compressed.data(), compressed.data() + s);
                edhttp::buffer_t const compressed_repeat(xz->decompress(broken_compressed_small));
                CATCH_REQUIRE(compressed_repeat == broken_compressed_small);
            }

            // we do recognize a xz buffer
            //
            CATCH_REQUIRE_FALSE(xz->compatible(input));
            CATCH_REQUIRE(xz->compatible(compressed));
            CATCH_REQUIRE_FALSE(xz->compatible(decompressed));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_xz: verify small buffers with xz compressor")
    {
        edhttp::compressor * xz(edhttp::get_compressor("xz"));
        CATCH_REQUIRE(xz != nullptr);

        // generate a random buffer to compress of 1kb to 16kb in size
        //
        for(int size(1); size < 20; ++size)
        {
            auto const input(SNAP_CATCH2_NAMESPACE::random_buffer(size, size));
            edhttp::buffer_t const compressed(xz->compress(input, rand() % 95 + 5, rand() & 1 == 0));

            bool equal(true);
            if(compressed.size() == input.size())
            {
                for(std::size_t pos(0); pos < compressed.size(); ++pos)
                {
                    if(compressed[pos] != input[pos])
                    {
                        equal = false;
                        break;
                    }
                }
            }
            else
            {
                equal = false;
            }
            CATCH_REQUIRE_FALSE(equal);

            edhttp::buffer_t const decompressed(xz->decompress(compressed));
            CATCH_REQUIRE(decompressed.size() == input.size());

            equal = true;
            for(std::size_t pos(0); pos < decompressed.size(); ++pos)
            {
                if(decompressed[pos] != input[pos])
                {
                    equal = false;
                    break;
                }
            }
            CATCH_REQUIRE(equal);

            // we do recognize a xz buffer
            //
            CATCH_REQUIRE(xz->compatible(compressed));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_xz: verify invalid xz magic length")
    {
        edhttp::compressor * xz(edhttp::get_compressor("xz"));
        CATCH_REQUIRE(xz != nullptr);
        CATCH_REQUIRE(strcmp(xz->get_name(), "xz") == 0);

        for(std::size_t size(0); size < 10; ++size)
        {
            // generate a random buffer to compress of 1kb to 16kb in size
            //
            auto input(SNAP_CATCH2_NAMESPACE::random_buffer(size, size));
            if(size >= 1)
            {
                input[0] = 0xFD;
            }
            if(size >= 2)
            {
                input[1] = '7';
            }
            if(size >= 3)
            {
                input[2] = 'z';
            }
            if(size >= 4)
            {
                input[3] = 'X';
            }
            if(size >= 5)
            {
                input[4] = 'Z';
            }
            CATCH_REQUIRE_FALSE(xz->compatible(input));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_xz: attempt xz compressing an empty buffer")
    {
        edhttp::compressor * xz(edhttp::get_compressor("xz"));
        CATCH_REQUIRE(xz != nullptr);
        CATCH_REQUIRE(strcmp(xz->get_name(), "xz") == 0);

        edhttp::buffer_t const empty;
        CATCH_REQUIRE_FALSE(xz->compatible(empty));

        for(edhttp::level_t level(0); level <= 120; level += rand() % 10 + 1)
        {
            edhttp::buffer_t const compressed(xz->compress(empty, level, false));

            // an empty buffer cannot be "compressed" (the output is bigger,
            // but it does not fail)
            //
            //CATCH_REQUIRE(empty == compressed);

            edhttp::buffer_t const decompressed(xz->decompress(compressed));
            CATCH_REQUIRE(empty == decompressed);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("compressor", "[compression]")
{
    CATCH_START_SECTION("compressor: verify list of compressors in our library")
    {
        advgetopt::string_list_t const list(edhttp::compressor_list());

        CATCH_REQUIRE(list.size() == 4);

        // internally it's in a map so it remains sorted
        //
        CATCH_REQUIRE(list[0] == "bz2");
        CATCH_REQUIRE(list[1] == "deflate");
        CATCH_REQUIRE(list[2] == "gzip");
        CATCH_REQUIRE(list[3] == "xz");

        for(auto const & name : list)
        {
            edhttp::compressor * c(edhttp::get_compressor(name));
            CATCH_REQUIRE(c != nullptr);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: verify the \"unknown\" compressor does not exist")
    {
        edhttp::compressor * unknown(edhttp::get_compressor("unknown"));
        CATCH_REQUIRE(unknown == nullptr);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress() with an empty input buffer")
    {
        // compress() with an empty buffer ignores the other parameters
        //
        advgetopt::string_list_t const list(edhttp::compressor_list());
        for(int i(0); i < 10; ++i)
        {
            edhttp::buffer_t const buffer;
            std::string compressor_name(list[rand() % list.size()]);
            edhttp::result_t const compressed(edhttp::compress({compressor_name}, buffer, rand() % 96 + 5, rand() % 1 == 0));
            CATCH_REQUIRE(compressed.first == buffer);
            CATCH_REQUIRE(compressed.second == edhttp::compressor::NO_COMPRESSION);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress() with too small a level")
    {
        // compress() with an empty buffer ignores the other parameters
        //
        advgetopt::string_list_t const list(edhttp::compressor_list());
        for(int i(0); i < 10; ++i)
        {
            edhttp::buffer_t const buffer(SNAP_CATCH2_NAMESPACE::random_buffer(1, 1024));
            std::string compressor_name(list[rand() % list.size()]);
            edhttp::result_t const compressed(edhttp::compress({compressor_name}, buffer, rand() % 5, rand() % 1 == 0));
            CATCH_REQUIRE(compressed.first == buffer);
            CATCH_REQUIRE(compressed.second == edhttp::compressor::NO_COMPRESSION);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress() with unknown compressor")
    {
        // compress() with an empty buffer ignores the other parameters
        //
        for(int i(0); i < 10; ++i)
        {
            edhttp::buffer_t const buffer(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024 * 16));
            std::string compressor_name("unknown");
            edhttp::result_t const compressed(edhttp::compress({compressor_name}, buffer, rand() % 96 + 5, rand() % 1 == 0));
            CATCH_REQUIRE(compressed.second == edhttp::compressor::NO_COMPRESSION);
            CATCH_REQUIRE(compressed.first == buffer);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress() small buffers with bz2 return input")
    {
        // compress() with an empty buffer ignores the other parameters
        //
        for(int i(1); i < 10; ++i)
        {
            edhttp::buffer_t const buffer(SNAP_CATCH2_NAMESPACE::random_buffer(1, i));
            std::string compressor_name("bz2");
            edhttp::result_t const compressed(edhttp::compress({compressor_name}, buffer, 100, rand() % 1 == 0));
            CATCH_REQUIRE(compressed.first == buffer);
            CATCH_REQUIRE(compressed.second == edhttp::compressor::NO_COMPRESSION);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress() small buffers with gzip return input")
    {
        // compress() with an empty buffer ignores the other parameters
        //
        for(int i(1); i < 10; ++i)
        {
            edhttp::buffer_t const buffer(SNAP_CATCH2_NAMESPACE::random_buffer(1, i));
            edhttp::result_t const compressed(edhttp::compress({"gzip"}, buffer, rand() % 96 + 5, rand() % 1 == 0));
            CATCH_REQUIRE(compressed.first == buffer);
            CATCH_REQUIRE(compressed.second == edhttp::compressor::NO_COMPRESSION);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress() small buffers with xz return input")
    {
        // compress() with a small buffer ignores the other parameters
        //
        for(int i(1); i < 10; ++i)
        {
            edhttp::buffer_t const buffer(SNAP_CATCH2_NAMESPACE::random_buffer(1, i));
            edhttp::result_t const compressed(edhttp::compress({"xz"}, buffer, rand() % 96 + 5, rand() % 1 == 0));
            CATCH_REQUIRE(compressed.first == buffer);
            CATCH_REQUIRE(compressed.second == edhttp::compressor::NO_COMPRESSION);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress() with deflate; decompress explicitly")
    {
        // get text because it compresses well and the test will work
        //
        snapdev::file_contents source(SNAP_CATCH2_NAMESPACE::g_source_dir() + "/tests/catch_compressor.cpp");
        CATCH_REQUIRE(source.read_all());
        std::string const data(source.contents());
        edhttp::buffer_t const buffer(data.begin(), data.end());
        edhttp::compressor * deflate(edhttp::get_compressor("deflate"));
        CATCH_REQUIRE(deflate != nullptr);
        CATCH_REQUIRE(strcmp(deflate->get_name(), "deflate") == 0);
        for(int i(1); i < 10; ++i)
        {
            std::string compressor_name("deflate");
            edhttp::result_t const compressed(edhttp::compress({compressor_name}, buffer, rand() % 96 + 5, true));
            CATCH_REQUIRE(compressed.second == "deflate");

            // deflate data as no magic, so calling decompress() "fails"
            //
            edhttp::result_t const decompressed(edhttp::decompress(compressed.first));
            CATCH_REQUIRE(decompressed.second == edhttp::compressor::NO_COMPRESSION);
            CATCH_REQUIRE(decompressed.first == compressed.first); // still compressed

            // instead we have to explicitly decompress
            //
            edhttp::buffer_t const original(deflate->decompress(compressed.first, buffer.size()));
            CATCH_REQUIRE(original == buffer);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress()/decompress() with bz2")
    {
        // get text because it compresses well and the test will work
        //
        snapdev::file_contents source(SNAP_CATCH2_NAMESPACE::g_source_dir() + "/tests/catch_compressor.cpp");
        CATCH_REQUIRE(source.read_all());
        std::string const data(source.contents());
        edhttp::buffer_t const buffer(data.begin(), data.end());
        edhttp::compressor * bz2(edhttp::get_compressor("bz2"));
        CATCH_REQUIRE(bz2 != nullptr);
        CATCH_REQUIRE(strcmp(bz2->get_name(), "bz2") == 0);
        for(int i(1); i < 10; ++i)
        {
            std::string compressor_name("bz2");
            edhttp::result_t const compressed(edhttp::compress({compressor_name}, buffer, rand() % 96 + 5, true));
            CATCH_REQUIRE(compressed.second == "bz2");
            CATCH_REQUIRE(bz2->compatible(compressed.first));
            CATCH_REQUIRE(compressed.first != buffer);
            edhttp::result_t const decompressed(edhttp::decompress(compressed.first));
            CATCH_REQUIRE(decompressed.second == "bz2");
            CATCH_REQUIRE(decompressed.first == buffer);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress()/decompress() a large buffer with bz2")
    {
        // get text because it compresses well and the test will work
        //
        snapdev::file_contents source(SNAP_CATCH2_NAMESPACE::g_source_dir() + "/tests/catch_compressor.cpp");
        CATCH_REQUIRE(source.read_all());
        std::string data(source.contents());
        while(data.length() < 1024 * 100 + 1)
        {
            data += data;
        }
        edhttp::buffer_t const buffer(data.begin(), data.end());
        edhttp::compressor * bz2(edhttp::get_compressor("bz2"));
        CATCH_REQUIRE(bz2 != nullptr);
        CATCH_REQUIRE(strcmp(bz2->get_name(), "bz2") == 0);
        for(int i(1); i < 10; ++i)
        {
            edhttp::result_t const compressed(edhttp::compress({"bz2"}, buffer, rand() % 96 + 5, true));
            CATCH_REQUIRE(compressed.second == "bz2");
            CATCH_REQUIRE(bz2->compatible(compressed.first));
            CATCH_REQUIRE(compressed.first != buffer);
            edhttp::result_t const decompressed(edhttp::decompress(compressed.first));
            CATCH_REQUIRE(decompressed.second == "bz2");
            CATCH_REQUIRE(decompressed.first == buffer);

            // also test with the size
            //
            edhttp::buffer_t const decompressed2(bz2->decompress(compressed.first, buffer.size()));
            CATCH_REQUIRE(decompressed2 == buffer);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress()/decompress() with gzip")
    {
        // get text because it compresses well and the test will work
        //
        snapdev::file_contents source(SNAP_CATCH2_NAMESPACE::g_source_dir() + "/tests/catch_compressor.cpp");
        CATCH_REQUIRE(source.read_all());
        std::string const data(source.contents());
        edhttp::buffer_t const buffer(data.begin(), data.end());
        edhttp::compressor * gzip(edhttp::get_compressor("gzip"));
        CATCH_REQUIRE(gzip != nullptr);
        CATCH_REQUIRE(strcmp(gzip->get_name(), "gzip") == 0);
        for(int i(1); i < 10; ++i)
        {
            std::string compressor_name("gzip");
            edhttp::result_t const compressed(edhttp::compress({compressor_name}, buffer, rand() % 96 + 5, true));
            CATCH_REQUIRE(compressed.second == "gzip");
            CATCH_REQUIRE(gzip->compatible(compressed.first));
            CATCH_REQUIRE(compressed.first != buffer);
            edhttp::result_t const decompressed(edhttp::decompress(compressed.first));
            CATCH_REQUIRE(decompressed.second == "gzip");
            CATCH_REQUIRE(decompressed.first == buffer);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress()/decompress() with best compressor")
    {
        // get text because it compresses well and the test works every time
        //
        snapdev::file_contents source(SNAP_CATCH2_NAMESPACE::g_source_dir() + "/tests/catch_compressor.cpp");
        CATCH_REQUIRE(source.read_all());
        std::string const data(source.contents());
        edhttp::buffer_t const buffer(data.begin(), data.end());
        for(int i(0); i < 10; ++i)
        {
            advgetopt::string_list_t names;
            if((i & 1) == 1)
            {
                names.push_back("bz2");
                names.push_back("gzip");
                std::random_device rd;
                std::mt19937 g(rd());
                std::shuffle(names.begin(), names.end(), g);
            }
            edhttp::result_t const compressed(edhttp::compress({}, buffer, rand() % 96 + 5, true));
            if(compressed.second == edhttp::compressor::NO_COMPRESSION)
            {
                // no compression possible
                //
                CATCH_REQUIRE(compressed.first == buffer);
            }
            else
            {
                edhttp::compressor * c(edhttp::get_compressor(compressed.second));
                CATCH_REQUIRE(c != nullptr);
                CATCH_REQUIRE(c->get_name() == compressed.second);
SNAP_LOG_WARNING << "--- c = " << c->get_name() << SNAP_LOG_SEND;
                if(compressed.second != "deflate")
                {
                    CATCH_REQUIRE(c->compatible(compressed.first));
                    edhttp::result_t const decompressed(edhttp::decompress(compressed.first));
                    CATCH_REQUIRE(decompressed.second == compressed.second);
                    CATCH_REQUIRE(decompressed.first == buffer);
                }
                else
                {
                    // deflate cannot be accessed from edhttp::decompress() because
                    // there is no magic and thus it cannot be a sure thing
                    //
                    edhttp::buffer_t const decompressed(c->decompress(compressed.first, buffer.size()));
                    CATCH_REQUIRE(decompressed == buffer);
                }
            }
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor: compress() too small a buffer with any compressor")
    {
        for(int i(0); i < 10; ++i)
        {
            // generate a very small random buffer to compress so that the
            // result is larger than the input and thus "fails" in a slightly
            // different path
            //
            edhttp::buffer_t const buffer(SNAP_CATCH2_NAMESPACE::random_buffer(1, 5));
            edhttp::result_t const compressed(edhttp::compress({}, buffer, rand() % 96 + 5, true));
            CATCH_REQUIRE(compressed.second == edhttp::compressor::NO_COMPRESSION);
            CATCH_REQUIRE(compressed.first == buffer);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("compressor_error", "[compression][error]")
{
    CATCH_START_SECTION("compressor_error: deflate decompress() requires a size")
    {
        edhttp::compressor * deflate(edhttp::get_compressor("deflate"));
        CATCH_REQUIRE(deflate != nullptr);
        CATCH_REQUIRE(strcmp(deflate->get_name(), "deflate") == 0);

        // generate a random buffer to compress of 1kb to 16kb in size
        //
        auto const buffer(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024 * 16));
        CATCH_REQUIRE_THROWS_MATCHES(
                  deflate->decompress(buffer)
                , edhttp::not_implemented
                , Catch::Matchers::ExceptionMessage(
                          "not_implemented: deflate::decompress() without the uncompressed_size parameter is not implemented."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_error: gzip decompress() does not support a size")
    {
        edhttp::compressor * gzip(edhttp::get_compressor("gzip"));
        CATCH_REQUIRE(gzip != nullptr);
        CATCH_REQUIRE(strcmp(gzip->get_name(), "gzip") == 0);

        // generate a random buffer to compress of 1kb to 16kb in size
        //
        auto const buffer(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024 * 16));
        CATCH_REQUIRE_THROWS_MATCHES(
                  gzip->decompress(buffer, buffer.size())
                , edhttp::not_implemented
                , Catch::Matchers::ExceptionMessage(
                          "not_implemented: gzip::decompress() with a size is not implemented."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_error: xz decompress() does not support a size")
    {
        edhttp::compressor * gzip(edhttp::get_compressor("xz"));
        CATCH_REQUIRE(gzip != nullptr);
        CATCH_REQUIRE(strcmp(gzip->get_name(), "xz") == 0);

        // generate a random buffer to compress of 1kb to 16kb in size
        //
        auto const buffer(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024 * 16));
        CATCH_REQUIRE_THROWS_MATCHES(
                  gzip->decompress(buffer, buffer.size())
                , edhttp::not_implemented
                , Catch::Matchers::ExceptionMessage(
                          "not_implemented: xz::decompress() with a size is not implemented."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_error: compressor name cannot be nullptr or empty")
    {
        CATCH_REQUIRE_THROWS_MATCHES(
                  new compressor_named(nullptr)
                , edhttp::invalid_token
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: the name of a compressor cannot be empty."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  new compressor_named("")
                , edhttp::invalid_token
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: the name of a compressor cannot be empty."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_error: compressor name cannot be a special name")
    {
        CATCH_REQUIRE_THROWS_MATCHES(
                  new compressor_named(edhttp::compressor::NO_COMPRESSION)
                , edhttp::incompatible
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: name \"none\" is not available as a compressor name."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("compressor_error: compressor name cannot include a special character")
    {
        // Note: we cannot test character '\0' since we use a C-string to
        //       name the compressor (so '\0' is end of string in this case)
        //
        for(int c(1); c < 256; ++c)
        {
            std::string name("name");
            name += c;
            if(!edhttp::is_token(name))
            {
                // if the following doesn't throw, then we have an issue
                // because the compressor will have a temporary string
                // as its name
                //
                CATCH_REQUIRE_THROWS_MATCHES(
                          new compressor_named(name.c_str())
                        , edhttp::invalid_token
                        , Catch::Matchers::ExceptionMessage(
                                  "edhttp_exception: a compressor name (\"" + name + "\") must be a valid HTTP token."));
            }
        }

        // also a token cannot start with the '$' character
        //
        CATCH_REQUIRE_THROWS_MATCHES(
                  new compressor_named("$name")
                , edhttp::invalid_token
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: a compressor name (\"$name\") must be a valid HTTP token."));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
