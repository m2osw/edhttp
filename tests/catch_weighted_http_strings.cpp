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
 * \brief Verify the weighted_http_string class.
 *
 * This file implements a tests to verify that the weighted_http_string
 * class functions as expected.
 */

// self
//
#include    "catch_main.h"


// edhttp
//
#include    "edhttp/exception.h"
#include    "edhttp/weighted_http_string.h"



CATCH_TEST_CASE("weighted_http_string", "[string]")
{
    CATCH_START_SECTION("weighted_http_strings: verify object, except parts")
    {
        edhttp::weighted_http_string locale("en");

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "en");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::string_part::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify parts")
    {
        edhttp::weighted_http_string locale("en");

        // make sure the result is 1 part
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 1);

        // also get the constant version
        //
        edhttp::string_part::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 1);

        // now verify that the part is correct
        //
        CATCH_REQUIRE(p[0].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify weights top functions")
    {
        edhttp::weighted_http_string locale("en-US,en;q=0.8,fr-FR;q= 0.5,fr;q =0.3");

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "en-US,en;q=0.8,fr-FR;q= 0.5,fr;q =0.3");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en-US"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"),    0.8f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr-FR"), 0.5f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"),    0.3f));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "en-US, en; q=0.8, fr-FR; q=0.5, fr; q=0.3");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify weight parts")
    {
        edhttp::weighted_http_string locale("en-US,en;q=\"0.8\",fr-FR;q=0.5,fr;q='0.3'");

        // make sure the result is 4 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 4);

        // also get the constant version
        //
        edhttp::string_part::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 4);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "en-US");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "en-US");

        CATCH_REQUIRE(p[1].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.8f));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "en; q=0.8");

        CATCH_REQUIRE(p[2].get_name() == "fr-FR");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.5f));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "fr-FR; q=0.5");

        CATCH_REQUIRE(p[3].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[3].get_level(), 0.3f));
        CATCH_REQUIRE(p[3].get_parameter("test") == "");
        CATCH_REQUIRE(p[3].to_string() == "fr; q=0.3");

        CATCH_REQUIRE(pc[0].get_name() == "en-US");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "en-US");

        CATCH_REQUIRE(pc[1].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), 0.8f));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "en; q=0.8");

        CATCH_REQUIRE(pc[2].get_name() == "fr-FR");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.5f));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "fr-FR; q=0.5");

        CATCH_REQUIRE(pc[3].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[3].get_level(), 0.3f));
        CATCH_REQUIRE(pc[3].get_parameter("test") == "");
        CATCH_REQUIRE(pc[3].to_string() == "fr; q=0.3");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: sort has no effect if weights are equal")
    {
        edhttp::weighted_http_string locale("en-US,en;q=0.8, fr-FR ; q = \"0.5\" ,fr;q=0.3");

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 4);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "en-US");
        CATCH_REQUIRE(p[1].get_name() == "en");
        CATCH_REQUIRE(p[2].get_name() == "fr-FR");
        CATCH_REQUIRE(p[3].get_name() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify object")
    {
        edhttp::weighted_http_string locale("de, en, fr");

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "de, en, fr");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::string_part::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "de, en, fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify part")
    {
        edhttp::weighted_http_string locale("de, en, fr");

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // also get the constant version
        //
        edhttp::string_part::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "de");

        CATCH_REQUIRE(p[1].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "en");

        CATCH_REQUIRE(p[2].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "fr");

        CATCH_REQUIRE(pc[0].get_name() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "de");

        CATCH_REQUIRE(pc[1].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "en");

        CATCH_REQUIRE(pc[2].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: sort has no effect if weights are equal")
    {
        edhttp::weighted_http_string locale("de, en, fr");

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "de");
        CATCH_REQUIRE(p[1].get_name() == "en");
        CATCH_REQUIRE(p[2].get_name() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify object")
    {
        edhttp::weighted_http_string locale("fr, za, en");

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "fr, za, en");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "fr, za, en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify part")
    {
        edhttp::weighted_http_string locale("fr, za, en");

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // also get the constant version
        //
        edhttp::string_part::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "fr");

        CATCH_REQUIRE(p[1].get_name() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "za");

        CATCH_REQUIRE(p[2].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "en");

        CATCH_REQUIRE(pc[0].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "fr");

        CATCH_REQUIRE(pc[1].get_name() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "za");

        CATCH_REQUIRE(pc[2].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: sort has no effect if weights are equal")
    {
        edhttp::weighted_http_string locale("fr, za, en");

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "fr");
        CATCH_REQUIRE(p[1].get_name() == "za");
        CATCH_REQUIRE(p[2].get_name() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify object")
    {
        edhttp::weighted_http_string locale("fr;q=0, za; q=0.6,en; q=0.4");

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "fr;q=0, za; q=0.6,en; q=0.4");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), 0.0f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), 0.6f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), 0.4f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "fr; q=0, za; q=0.6, en; q=0.4");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify part after copy in non-const and in const")
    {
        edhttp::weighted_http_string locale("fr;q=0, za; q=0.6,en; q=0.4");

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // also get the constant version
        //
        edhttp::string_part::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(p[1].get_name() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "za; q=0.6");

        CATCH_REQUIRE(p[2].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.4f));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "en; q=0.4");

        CATCH_REQUIRE(pc[0].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), 0.0f));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(pc[1].get_name() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), 0.6f));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "za; q=0.6");

        CATCH_REQUIRE(pc[2].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.4f));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "en; q=0.4");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: sort by level (weight)")
    {
        edhttp::weighted_http_string locale("fr;q=0, za; q=0.6,en; q=0.4");

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are sorted by level
        //    "fr; q=0, za; q=0.6, en; q=0.4"
        //
        CATCH_REQUIRE(p[0].get_name() == "za");
        CATCH_REQUIRE(p[1].get_name() == "en");
        CATCH_REQUIRE(p[2].get_name() == "fr");

        // convert back to a string in the new order and with spaces
        //
        CATCH_REQUIRE(locale.to_string() == "za; q=0.6, en; q=0.4, fr; q=0");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify object")
    {
        edhttp::weighted_http_string locale("  fr;  q=0,  za;  q=0.6,  en;  q=0.4  ");

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "  fr;  q=0,  za;  q=0.6,  en;  q=0.4  ");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), 0.0f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), 0.6f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), 0.4f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), -1.0f));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "fr; q=0, za; q=0.6, en; q=0.4");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify part")
    {
        edhttp::weighted_http_string locale("  fr;  q=0,  za;  q=0.6,  en;  q=0.4  ");

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // also get the constant version
        //
        edhttp::string_part::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(p[1].get_name() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "za; q=0.6");

        CATCH_REQUIRE(p[2].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.4f));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "en; q=0.4");

        CATCH_REQUIRE(pc[0].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), 0.0f));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(pc[1].get_name() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), 0.6f));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "za; q=0.6");

        CATCH_REQUIRE(pc[2].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.4f));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "en; q=0.4");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: sort by level (weight)")
    {
        edhttp::weighted_http_string locale("  fr;  q=0,  za;  q=0.6,  en;  q=0.4  ");

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are sorted by level
        //
        CATCH_REQUIRE(p[0].get_name() == "za");
        CATCH_REQUIRE(p[1].get_name() == "en");
        CATCH_REQUIRE(p[2].get_name() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify object")
    {
        edhttp::weighted_http_string locale("  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0");

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), 0.0f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), 0.4f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), 1.0f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), -1.0f));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "fr; q=0, za, en; q=0.4, es; q=1.0");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify part")
    {
        edhttp::weighted_http_string locale("  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0");

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 4);

        // also get the constant version
        //
        edhttp::string_part::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 4);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(p[1].get_name() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "za");

        CATCH_REQUIRE(p[2].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.4f));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "en; q=0.4");

        CATCH_REQUIRE(p[3].get_name() == "es");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[3].get_level(), 1.0f));
        CATCH_REQUIRE(p[3].get_parameter("test") == "");
        CATCH_REQUIRE(p[3].to_string() == "es; q=1.0");

        CATCH_REQUIRE(pc[0].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), 0.0f));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(pc[1].get_name() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "za");

        CATCH_REQUIRE(pc[2].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.4f));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "en; q=0.4");

        CATCH_REQUIRE(p[3].get_name() == "es");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[3].get_level(), 1.0f));
        CATCH_REQUIRE(p[3].get_parameter("test") == "");
        CATCH_REQUIRE(p[3].to_string() == "es; q=1.0");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: sort by level (weight)")
    {
        edhttp::weighted_http_string locale("  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0");

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 4);

        // now verify that the parts are sorted by level
        //
        CATCH_REQUIRE(p[0].get_name() == "za");
        CATCH_REQUIRE(p[1].get_name() == "es");
        CATCH_REQUIRE(p[2].get_name() == "en");
        CATCH_REQUIRE(p[3].get_name() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify object")
    {
        edhttp::weighted_http_string locale("de");

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "de");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::string_part::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "de");

        // make sure the result is 1 part
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 1);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "de");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: add \"en\"")
    {
        edhttp::weighted_http_string locale("de");

        // the parse is expected to work (return true)
        //
        CATCH_REQUIRE(locale.parse("en"));

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "de,en");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::string_part::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "de, en");

        // make sure the result is 2 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 2);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "de");

        CATCH_REQUIRE(p[1].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: add \"en\" and then \"fr\"")
    {
        edhttp::weighted_http_string locale("de");

        // the parse is expected to work (return true)
        //
        CATCH_REQUIRE(locale.parse("en"));
        CATCH_REQUIRE(locale.parse("fr"));

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "de,en,fr");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::string_part::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "de, en, fr");

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "de");

        CATCH_REQUIRE(p[1].get_name() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "en");

        CATCH_REQUIRE(p[2].get_name() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: replace with \"mo\"")
    {
        edhttp::weighted_http_string locale("de");

        // the parse is expected to work (return true)
        //
        CATCH_REQUIRE(locale.parse("  mo  ", true));

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "  mo  ");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("mo"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::string_part::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "mo");

        // make sure the result is 1 part
        //
        edhttp::string_part::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 1);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "mo");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "mo");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: set parameters with a value")
    {
        edhttp::weighted_http_string locale("de=123");
        CATCH_REQUIRE(locale.error_messages().empty());
        CATCH_REQUIRE(locale.get_string() == "de=123");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("mo"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(locale.to_string() == "de=123");

        CATCH_REQUIRE(locale.parse("  mo  =  \"555\"  ", true));
        CATCH_REQUIRE(locale.error_messages().empty());
        CATCH_REQUIRE(locale.get_string() == "  mo  =  \"555\"  ");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("mo"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(locale.to_string() == "mo=555");

        CATCH_REQUIRE(locale.parse("  en  =  '  555  '  ", true));
        CATCH_REQUIRE(locale.error_messages().empty());
        CATCH_REQUIRE(locale.get_string() == "  en  =  '  555  '  ");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("mo"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(locale.to_string() == "en=\"  555  \"");

        CATCH_REQUIRE(locale.parse("  es  =  555  ", true));
        CATCH_REQUIRE(locale.error_messages().empty());
        CATCH_REQUIRE(locale.get_string() == "  es  =  555  ");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("mo"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::string_part::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(locale.to_string() == "es=555");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify multiple parts")
    {
        edhttp::weighted_http_string locale("fr;q=0;r=3.2;z=fancy, za; q = 0.6 ; h = \"angry\" ; object = 'color of the wand',en; f=0.4");

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t const & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "fr");
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        CATCH_REQUIRE(p[0].get_parameter("r") == "3.2");
        CATCH_REQUIRE(p[0].get_parameter("z") == "fancy");
        CATCH_REQUIRE(p[0].to_string() == "fr; q=0; r=3.2; z=fancy");

        CATCH_REQUIRE(p[1].get_name() == "za");
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
        CATCH_REQUIRE(p[1].get_parameter("h") == "angry");
        CATCH_REQUIRE(p[1].get_parameter("object") == "color of the wand");
        CATCH_REQUIRE(p[1].to_string() == "za; h=angry; object=\"color of the wand\"; q=0.6");

        CATCH_REQUIRE(p[2].get_name() == "en");
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("f") == "0.4");
        CATCH_REQUIRE(p[2].to_string() == "en; f=0.4");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("weighted_http_strings: verify other quotation requirements")
    {
        edhttp::weighted_http_string locale("fr=francais;q=0;r=3.2;z=\"c'est necessaire\", za=\"South Africa\"; q = 0.6 ; h = \"angry\" ; object = 'color of the wand',en; f=0.4; cute='girls \"are\" dancing'");

        // make sure the result is 3 parts
        //
        edhttp::string_part::vector_t const & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "fr");
        CATCH_REQUIRE(p[0].get_value() == "francais");
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        CATCH_REQUIRE(p[0].get_parameter("r") == "3.2");
        CATCH_REQUIRE(p[0].get_parameter("z") == "c'est necessaire");
        CATCH_REQUIRE(p[0].to_string() == "fr=francais; q=0; r=3.2; z=\"c'est necessaire\"");

        CATCH_REQUIRE(p[1].get_name() == "za");
        CATCH_REQUIRE(p[1].get_value() == "South Africa");
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
        CATCH_REQUIRE(p[1].get_parameter("h") == "angry");
        CATCH_REQUIRE(p[1].get_parameter("object") == "color of the wand");
        CATCH_REQUIRE(p[1].to_string() == "za=\"South Africa\"; h=angry; object=\"color of the wand\"; q=0.6");

        CATCH_REQUIRE(p[2].get_name() == "en");
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::string_part::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("f") == "0.4");
        CATCH_REQUIRE(p[2].get_parameter("cute") == "girls \"are\" dancing");
        CATCH_REQUIRE(p[2].to_string() == "en; cute='girls \"are\" dancing'; f=0.4");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("invalid_weighted_http_string", "[invalid][string]")
{
    CATCH_START_SECTION("invalid_weighted_http_strings: main name too long")
    {
        edhttp::weighted_http_string locale;

        CATCH_REQUIRE_FALSE(locale.parse("deutsch_ist_zu-schwierig"));

        CATCH_REQUIRE(locale.error_messages() == "part name is empty or too long (limit is '8-8' characters).\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_weighted_http_strings: sub-name too long")
    {
        edhttp::weighted_http_string locale;

        CATCH_REQUIRE_FALSE(locale.parse("deutsch-ist_zu_schwierig"));

        CATCH_REQUIRE(locale.error_messages() == "part sub-name is empty or too long (limit is '8-8' characters).\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_weighted_http_strings: too many dashes in part name")
    {
        edhttp::weighted_http_string locale;

        CATCH_REQUIRE_FALSE(locale.parse("deutsch-ist-zu-schwierig"));

        CATCH_REQUIRE(locale.error_messages() == "part name cannot include more than one '-'.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_weighted_http_strings: negative quality is not acceptable")
    {
        edhttp::weighted_http_string locale;

        CATCH_REQUIRE_FALSE(locale.parse("fr-FR;q=-1.0"));

        CATCH_REQUIRE(locale.error_messages() == "the quality value (q=...) cannot be a negative number.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_weighted_http_strings: quality must be a valid double")
    {
        edhttp::weighted_http_string locale;

        CATCH_REQUIRE_FALSE(locale.parse("fr-FR;q=joke"));

        CATCH_REQUIRE(locale.error_messages() == "the quality value (q=...) is not a valid floating point.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_weighted_http_strings: spurious data")
    {
        edhttp::weighted_http_string locale;

        CATCH_REQUIRE_FALSE(locale.parse("fr-FR;joke=\"it is\" not"));

        CATCH_REQUIRE(locale.error_messages() == "found a spurious character in a weighted string.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_weighted_http_strings: expected value separator (,) or EOS")
    {
        edhttp::weighted_http_string locale;

        // TODO: the space should not be require to get the error!
        CATCH_REQUIRE_FALSE(locale.parse("fr-FR |"));

        CATCH_REQUIRE(locale.error_messages() == "part not ended by a comma or end of string.\n");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("invalid_string_part", "[invalid][string]")
{
    CATCH_START_SECTION("invalid_string_part: part value includes ' and \"")
    {
        edhttp::string_part p("invalid");
        CATCH_REQUIRE(p.get_name() == "invalid");
        CATCH_REQUIRE(p.get_value().empty());
        p.set_value("c'est pas \"possible\"");
        CATCH_REQUIRE(p.get_value() == "c'est pas \"possible\"");
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.to_string()
                , edhttp::unquotable_string
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: string [c'est pas \"possible\"] includes single and double quotes."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_string_part: part value includes \" and '")
    {
        edhttp::string_part p("invalid");
        CATCH_REQUIRE(p.get_name() == "invalid");
        CATCH_REQUIRE(p.get_value().empty());
        p.set_value("\"c'est pas possible\"");
        CATCH_REQUIRE(p.get_value() == "\"c'est pas possible\"");
        CATCH_REQUIRE_THROWS_MATCHES(
                  p.to_string()
                , edhttp::unquotable_string
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: string [\"c'est pas possible\"] includes single and double quotes."));
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
