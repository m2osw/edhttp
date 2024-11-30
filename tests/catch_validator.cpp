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

/** \file
 * \brief Verify the validator_uri class.
 *
 * This file implements tests to verify that the validator_uri
 * class functions as expected.
 */

// self
//
#include    "catch_main.h"


// edhttp
//
#include    <edhttp/validator_uri.h>



CATCH_TEST_CASE("validator_uri", "[domain]")
{
    CATCH_START_SECTION("validator_uri: validate valid and invalid URIs (default)")
    {
        advgetopt::validator::pointer_t v(advgetopt::validator::create("uri"));

        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->name() == "uri");

        CATCH_REQUIRE(v->validate("http://snap.website/"));
        CATCH_REQUIRE(v->validate("https://snap.website/with/path"));
        CATCH_REQUIRE(v->validate("gopher://snap.website/?with=param"));
        CATCH_REQUIRE(v->validate("cd://snap.website/#with-anchor"));

        CATCH_REQUIRE_FALSE(v->validate("://bar.uri/"));
        CATCH_REQUIRE_FALSE(v->validate("bad:too"));
        CATCH_REQUIRE_FALSE(v->validate("---really bad---"));

        CATCH_REQUIRE_FALSE(v->validate("http://127.0.0.1/refused-by-default"));
        CATCH_REQUIRE_FALSE(v->validate("cdu:/refused/by/default"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator_uri: validate valid and invalid URIs (refuse all)")
    {
        advgetopt::validator::pointer_t v(advgetopt::validator::create("uri(refuse-ip, refuse-path)"));

        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->name() == "uri");

        CATCH_REQUIRE(v->validate("http://snap.website/"));
        CATCH_REQUIRE(v->validate("https://snap.website/with/path"));
        CATCH_REQUIRE(v->validate("gopher://snap.website/?with=param"));
        CATCH_REQUIRE(v->validate("cd://snap.website/#with-anchor"));

        CATCH_REQUIRE_FALSE(v->validate("://bar.uri/"));
        CATCH_REQUIRE_FALSE(v->validate("bad:too"));
        CATCH_REQUIRE_FALSE(v->validate("---really bad---"));

        CATCH_REQUIRE_FALSE(v->validate("http://127.0.0.1/refused-by-default"));
        CATCH_REQUIRE_FALSE(v->validate("cdu:/refused/by/default"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator_uri: validate valid and invalid URIs (refuse ip)")
    {
        advgetopt::validator::pointer_t v(advgetopt::validator::create("uri(refuse_ip)"));

        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->name() == "uri");

        CATCH_REQUIRE(v->validate("http://snap.website/"));
        CATCH_REQUIRE(v->validate("https://snap.website/with/path"));
        CATCH_REQUIRE(v->validate("gopher://snap.website/?with=param"));
        CATCH_REQUIRE(v->validate("cd://snap.website/#with-anchor"));

        CATCH_REQUIRE_FALSE(v->validate("://bar.uri/"));
        CATCH_REQUIRE_FALSE(v->validate("bad:too"));
        CATCH_REQUIRE_FALSE(v->validate("---really bad---"));

        CATCH_REQUIRE_FALSE(v->validate("http://127.0.0.1/refused-by-default"));
        CATCH_REQUIRE_FALSE(v->validate("cdu:/refused/by/default"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator_uri: validate valid and invalid URIs (refuse path)")
    {
        advgetopt::validator::pointer_t v(advgetopt::validator::create("uri(refuse_path)"));

        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->name() == "uri");

        CATCH_REQUIRE(v->validate("http://snap.website/"));
        CATCH_REQUIRE(v->validate("https://snap.website/with/path"));
        CATCH_REQUIRE(v->validate("gopher://snap.website/?with=param"));
        CATCH_REQUIRE(v->validate("cd://snap.website/#with-anchor"));

        CATCH_REQUIRE_FALSE(v->validate("://bar.uri/"));
        CATCH_REQUIRE_FALSE(v->validate("bad:too"));
        CATCH_REQUIRE_FALSE(v->validate("---really bad---"));

        CATCH_REQUIRE_FALSE(v->validate("http://127.0.0.1/refused-by-default"));
        CATCH_REQUIRE_FALSE(v->validate("cdu:/refused/by/default"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator_uri: validate valid and invalid URIs (accept IP, dash)")
    {
        advgetopt::validator::pointer_t v(advgetopt::validator::create("uri(accept-ip)"));

        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->name() == "uri");

        CATCH_REQUIRE(v->validate("http://snap.website/"));
        CATCH_REQUIRE(v->validate("https://snap.website/with/path"));
        CATCH_REQUIRE(v->validate("gopher://snap.website/?with=param"));
        CATCH_REQUIRE(v->validate("cd://snap.website/#with-anchor"));
        CATCH_REQUIRE(v->validate("http://127.0.0.1/accepted?this#one"));

        CATCH_REQUIRE_FALSE(v->validate("://bar.uri/"));
        CATCH_REQUIRE_FALSE(v->validate("bad:too"));
        CATCH_REQUIRE_FALSE(v->validate("---really bad---"));

        CATCH_REQUIRE_FALSE(v->validate("/refused/by/default"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator_uri: validate valid and invalid URIs (accept IP, underscore)")
    {
        advgetopt::validator::pointer_t v(advgetopt::validator::create("uri(accept_ip)"));

        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->name() == "uri");

        CATCH_REQUIRE(v->validate("http://snap.website/"));
        CATCH_REQUIRE(v->validate("https://snap.website/with/path"));
        CATCH_REQUIRE(v->validate("gopher://snap.website/?with=param"));
        CATCH_REQUIRE(v->validate("cd://snap.website/#with-anchor"));
        CATCH_REQUIRE(v->validate("http://127.0.0.1/accepted?this#one"));

        CATCH_REQUIRE_FALSE(v->validate("://bar.uri/"));
        CATCH_REQUIRE_FALSE(v->validate("bad:too"));
        CATCH_REQUIRE_FALSE(v->validate("---really bad---"));

        CATCH_REQUIRE_FALSE(v->validate("/refused/by/default"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator_uri: validate valid and invalid URIs (accept Unix path, dash)")
    {
        advgetopt::validator::pointer_t v(advgetopt::validator::create("uri(accept-path)"));

        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->name() == "uri");

        CATCH_REQUIRE(v->validate("http://snap.website/"));
        CATCH_REQUIRE(v->validate("https://snap.website/with/path"));
        CATCH_REQUIRE(v->validate("gopher://snap.website/?with=param"));
        CATCH_REQUIRE(v->validate("cd://snap.website/#with-anchor"));
        CATCH_REQUIRE(v->validate("cdu:///refused/by/default"));

        CATCH_REQUIRE_FALSE(v->validate("://bar.uri/"));
        CATCH_REQUIRE_FALSE(v->validate("bad:too"));
        CATCH_REQUIRE_FALSE(v->validate("---really bad---"));

        CATCH_REQUIRE_FALSE(v->validate("http://127.0.0.1/refused-by-default"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator_uri: validate valid and invalid URIs (accept Unix path, underscore)")
    {
        advgetopt::validator::pointer_t v(advgetopt::validator::create("uri(accept_path)"));

        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->name() == "uri");

        CATCH_REQUIRE(v->validate("http://snap.website/"));
        CATCH_REQUIRE(v->validate("https://snap.website/with/path"));
        CATCH_REQUIRE(v->validate("gopher://snap.website/?with=param"));
        CATCH_REQUIRE(v->validate("cd://snap.website/#with-anchor"));
        CATCH_REQUIRE(v->validate("cdu:///refused/by/default"));

        CATCH_REQUIRE_FALSE(v->validate("://bar.uri/"));
        CATCH_REQUIRE_FALSE(v->validate("bad:too"));
        CATCH_REQUIRE_FALSE(v->validate("---really bad---"));

        CATCH_REQUIRE_FALSE(v->validate("http://127.0.0.1/refused-by-default"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("validator_uri: validate valid and invalid URIs (invalid parameter)")
    {
        // TODO: verify error message
        advgetopt::validator::pointer_t v(advgetopt::validator::create("uri(allow_path)"));

        CATCH_REQUIRE(v != nullptr);
        CATCH_REQUIRE(v->name() == "uri");

        CATCH_REQUIRE(v->validate("http://snap.website/"));
        CATCH_REQUIRE(v->validate("https://snap.website/with/path"));
        CATCH_REQUIRE(v->validate("gopher://snap.website/?with=param"));
        CATCH_REQUIRE(v->validate("cd://snap.website/#with-anchor"));

        CATCH_REQUIRE_FALSE(v->validate("://bar.uri/"));
        CATCH_REQUIRE_FALSE(v->validate("bad:too"));
        CATCH_REQUIRE_FALSE(v->validate("---really bad---"));

        CATCH_REQUIRE_FALSE(v->validate("http://127.0.0.1/refused-by-default"));
        CATCH_REQUIRE_FALSE(v->validate("cdu:///refused/by/default"));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
