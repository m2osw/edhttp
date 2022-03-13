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

// self
//
#include    "catch_main.h"

// edhttp
//
#include    <edhttp/uri.h>



CATCH_TEST_CASE("uri", "[domain]")
{
    // "normal" (canonicalized) URI
    CATCH_GIVEN("domain \"http://snap.website/\"")
    {
        edhttp::uri uri("http://snap.website/");

        CATCH_SECTION("verify object")
        {
            CATCH_REQUIRE(uri.domain() == "snap");
            CATCH_REQUIRE(uri.top_level_domain() == ".website");
        }
    }

    // without a '/' after domain
    CATCH_GIVEN("domain \"http://snap.website\"")
    {
        edhttp::uri uri("http://snap.website");

        CATCH_SECTION("verify object")
        {
            CATCH_REQUIRE(uri.domain() == "snap");
            CATCH_REQUIRE(uri.top_level_domain() == ".website");
        }
    }

    // with two slashes
    CATCH_GIVEN("domain \"http://snap.website//\"")
    {
        edhttp::uri uri("http://snap.website//");

        CATCH_SECTION("verify object")
        {
            CATCH_REQUIRE(uri.domain() == "snap");
            CATCH_REQUIRE(uri.top_level_domain() == ".website");
        }
    }

    // multiple slashes and a path
    CATCH_GIVEN("domain \"http://snap.website///and/a/path\"")
    {
        edhttp::uri uri("http://snap.website///and/a/path");

        CATCH_SECTION("verify object")
        {
            CATCH_REQUIRE(uri.domain() == "snap");
            CATCH_REQUIRE(uri.top_level_domain() == ".website");
            CATCH_REQUIRE(uri.path() == "and/a/path");
        }
    }
}


// vim: ts=4 sw=4 et
