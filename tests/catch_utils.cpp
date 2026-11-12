// Copyright (c) 2011-2026  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Verify the token class and functions.
 *
 * This file implements tests to verify that the token class functions
 * properly. It also includes tests for the standalone functions.
 */

// edhttp
//
#include    <edhttp/utils.h>

#include    <edhttp/exception.h>


// self
//
#include    "catch_main.h"


// snaplogger
//
#include    <snaplogger/message.h>



CATCH_TEST_CASE("utils", "[string]")
{
    CATCH_START_SECTION("utils: decode percents")
    {
        struct in_out_t
        {
            std::string     f_in = std::string();
            std::string     f_out = std::string();
        };
        in_out_t in_out[]
        {
            {
                .f_in = "test",
                .f_out = "test",
            },
            {
                .f_in = "tab%09",
                .f_out = "tab\t",
            },
            {
                .f_in = "%283%20+%201%29%20*%205",
                .f_out = "(3 + 1) * 5",
            },
        };
        for(auto const & io : in_out)
        {
            std::string out;
            CATCH_REQUIRE(edhttp::decode_percents(io.f_in, out));
            CATCH_REQUIRE(io.f_out == out);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("utils_protocol", "[protocol]")
{
    CATCH_START_SECTION("utils_protocol: check that adding a protocol to the cache makes it work")
    {
        // at this time, this captures the return on a nullptr but there
        // is not really any guarantee that would remain true
        //
        CATCH_REQUIRE_FALSE(edhttp::accepted_protocol("utils:not-set"));

        edhttp::accepted_protocol("utils:www.example.com", 3600);
        CATCH_REQUIRE(edhttp::accepted_protocol("utils:www.example.com"));

        // never added so it returns false
        //
        CATCH_REQUIRE_FALSE(edhttp::accepted_protocol("utils:unknown.example.com"));
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("utils_error", "[string][error]")
{
    CATCH_START_SECTION("utils_error: invalid percent decoding")
    {
        std::string out;
        CATCH_REQUIRE_FALSE(edhttp::decode_percents("%error", out));
        CATCH_REQUIRE_FALSE(edhttp::decode_percents("%ff", out));
        CATCH_REQUIRE_FALSE(edhttp::decode_percents("%0f", out));
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
