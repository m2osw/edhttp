// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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

// edhttp
//
#include    <edhttp/version.h>


// self
//
#include    "catch_main.h"


// last include
//
#include    <snapdev/poison.h>



CATCH_TEST_CASE("version", "[version][valid]")
{
    CATCH_START_SECTION("Verify the test version")
        CATCH_REQUIRE(edhttp::get_major_version() == EDHTTP_VERSION_MAJOR);
        CATCH_REQUIRE(edhttp::get_release_version() == EDHTTP_VERSION_MINOR);
        CATCH_REQUIRE(edhttp::get_patch_version() == EDHTTP_VERSION_PATCH);
        CATCH_REQUIRE(strcmp(edhttp::get_version_string(), EDHTTP_VERSION_STRING) == 0);
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
