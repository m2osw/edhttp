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
#include    <edhttp/token.h>

#include    <edhttp/exception.h>


// self
//
#include    "catch_main.h"


// snapdev
//
//#include    <snapdev/tokenize_string.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
#include    <string.h>



CATCH_TEST_CASE("token", "[token]")
{
    CATCH_START_SECTION("token: verify simple tokens")
    {
        CATCH_REQUIRE(edhttp::is_token("snap"));
        CATCH_REQUIRE(edhttp::is_token("!not"));
        CATCH_REQUIRE(edhttp::is_token("#hash"));
        CATCH_REQUIRE(edhttp::is_token("$dollar"));
        CATCH_REQUIRE(edhttp::is_token("%percent"));
        CATCH_REQUIRE(edhttp::is_token("&amp"));
        CATCH_REQUIRE(edhttp::is_token("'apostrophe"));
        CATCH_REQUIRE(edhttp::is_token("*asterisk"));
        CATCH_REQUIRE(edhttp::is_token("+plus"));
        CATCH_REQUIRE(edhttp::is_token("-minus"));
        CATCH_REQUIRE(edhttp::is_token(".dot"));
        CATCH_REQUIRE(edhttp::is_token("^caret"));
        CATCH_REQUIRE(edhttp::is_token("_underscore"));
        CATCH_REQUIRE(edhttp::is_token("`backtick"));
        CATCH_REQUIRE(edhttp::is_token("|pipe"));
        CATCH_REQUIRE(edhttp::is_token("~tilde"));
        CATCH_REQUIRE(edhttp::is_token("num0"));
        CATCH_REQUIRE(edhttp::is_token("num1"));
        CATCH_REQUIRE(edhttp::is_token("num2"));
        CATCH_REQUIRE(edhttp::is_token("num3"));
        CATCH_REQUIRE(edhttp::is_token("num4"));
        CATCH_REQUIRE(edhttp::is_token("num5"));
        CATCH_REQUIRE(edhttp::is_token("num6"));
        CATCH_REQUIRE(edhttp::is_token("num7"));
        CATCH_REQUIRE(edhttp::is_token("num8"));
        CATCH_REQUIRE(edhttp::is_token("num9"));
        CATCH_REQUIRE(edhttp::is_token("ALL-CAPS"));
        CATCH_REQUIRE(edhttp::is_token("Flipped~Flopped"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("token: verify restricted tokens")
    {
        CATCH_REQUIRE(edhttp::is_restricted_token("snap"));
        CATCH_REQUIRE(edhttp::is_restricted_token("-minus"));
        CATCH_REQUIRE(edhttp::is_restricted_token("_underscore"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num0"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num1"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num2"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num3"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num4"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num5"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num6"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num7"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num8"));
        CATCH_REQUIRE(edhttp::is_restricted_token("num9"));
        CATCH_REQUIRE(edhttp::is_restricted_token("ALL-CAPS"));
        CATCH_REQUIRE(edhttp::is_restricted_token("Flipped_Flopped"));

        CATCH_REQUIRE_FALSE(edhttp::is_restricted_token("Flipped~Flopped"));
        CATCH_REQUIRE_FALSE(edhttp::is_restricted_token("[enclosed]"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("token: compare tokens")
    {
        CATCH_REQUIRE(edhttp::compare_tokens("snap", "websites", false) == -1);
        CATCH_REQUIRE(edhttp::compare_tokens("SNAP", "websites", false) == -1);
        CATCH_REQUIRE(edhttp::compare_tokens("websites", "Snap", false) == 1);
        CATCH_REQUIRE(edhttp::compare_tokens("Websites", "snap", false) == 1);

        CATCH_REQUIRE(edhttp::compare_tokens("follow-up", "follow_up", false) == -1);
        CATCH_REQUIRE(edhttp::compare_tokens("follow-up", "follow_up", true) == 0);
        CATCH_REQUIRE(edhttp::compare_tokens("follow_up", "follow-up", false) == 1);
        CATCH_REQUIRE(edhttp::compare_tokens("follow_up", "follow-up", true) == 0);

        CATCH_REQUIRE(edhttp::compare_tokens("follow-", "follow_up", true) == -1);
        CATCH_REQUIRE(edhttp::compare_tokens("follow-up", "follow_", true) == 1);
        CATCH_REQUIRE(edhttp::compare_tokens("follow_", "follow-up", true) == -1);
        CATCH_REQUIRE(edhttp::compare_tokens("follow_up", "follow-", true) == 1);
        CATCH_REQUIRE(edhttp::compare_tokens("follow-", "follow_up", false) == -1);
        CATCH_REQUIRE(edhttp::compare_tokens("follow-up", "follow_", false) == -1);
        CATCH_REQUIRE(edhttp::compare_tokens("follow_", "follow-up", false) == 1);
        CATCH_REQUIRE(edhttp::compare_tokens("follow_up", "follow-", false) == 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("token: token objects")
    {
        edhttp::token t1("this-is-a-token");
        edhttp::token const t2("another-token");
        edhttp::token t3(t2);

        CATCH_REQUIRE(t1.is_valid());
        CATCH_REQUIRE(t2.is_valid());
        CATCH_REQUIRE(t3.is_valid());
        CATCH_REQUIRE(t1 > t2);
        CATCH_REQUIRE(t1 >= t2);
        CATCH_REQUIRE(t2 < t1);
        CATCH_REQUIRE(t2 <= t1);
        CATCH_REQUIRE(t2 == t3);
        CATCH_REQUIRE(t3 == t2);

        edhttp::token t4;
        t4 = t1;
        CATCH_REQUIRE(t4 > t3);
        CATCH_REQUIRE(t4 == t1);
        CATCH_REQUIRE(t4 != t2);

        std::string s("colorful");
        edhttp::token const t5(s);
        CATCH_REQUIRE(t5 != t4);
        CATCH_REQUIRE(t5 < t4);

        CATCH_REQUIRE(t5 == s);
        CATCH_REQUIRE(s == t5);
        CATCH_REQUIRE(s <= t4);
        CATCH_REQUIRE(t4 >= s);
        CATCH_REQUIRE(t4 != s);
        CATCH_REQUIRE(s != t4);

        edhttp::token t6("test-a-long-token-with-the-move-operator");
        CATCH_REQUIRE(t6 == "test-a-long-token-with-the-move-operator");
        edhttp::token t7(std::move(t6));
        CATCH_REQUIRE(t7 == "test-a-long-token-with-the-move-operator");
        CATCH_REQUIRE(t6.empty());

        std::string org("this-long-token-name-can-be-copied-from-a-string");
        edhttp::token t8(std::move(org));
        CATCH_REQUIRE(t8 == "this-long-token-name-can-be-copied-from-a-string");
        CATCH_REQUIRE(org.empty());
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("token_error", "[token][error]")
{
    CATCH_START_SECTION("token_error: empty strings generate an error")
    {
        CATCH_REQUIRE_THROWS_MATCHES(
                  edhttp::is_token(std::string())
                , edhttp::invalid_token
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: an HTTP token cannot be empty."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("token_error: compare invalid tokens")
    {
        CATCH_REQUIRE_THROWS_MATCHES(
                  edhttp::compare_tokens("co lor", "color")
                , edhttp::invalid_token
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: one of the input token includes an invalid character."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  edhttp::compare_tokens("color", "co lor")
                , edhttp::invalid_token
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: one of the input token includes an invalid character."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("token_error: compare invalid tokens")
    {
        CATCH_REQUIRE_THROWS_MATCHES(
                  edhttp::is_restricted_token(std::string())
                , edhttp::invalid_token
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: a restricted HTTP token cannot be empty."));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
