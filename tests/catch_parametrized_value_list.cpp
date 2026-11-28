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
 * \brief Verify the parametrized_value_list class.
 *
 * This file implements tests to verify that the parametrized_value_list
 * class functions as expected.
 */

// header begin tested
//
#include    <edhttp/parametrized_value_list.h>



// self
//
#include    "catch_main.h"


// edhttp
//
#include    <edhttp/exception.h>
#include    <edhttp/uri.h>
#include    <edhttp/utils.h>



CATCH_TEST_CASE("parametrized_value_list", "[string]")
{
    CATCH_START_SECTION("parametrized_value_list: verify object, except parts")
    {
        edhttp::parametrized_value_list locale("en", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "en");

        // get_level() with correct and wrong names
        //
std::cerr << "--- level = [" << locale.get_level("en") << "/" << edhttp::parametrized_value::DEFAULT_LEVEL() << "]\n";
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::parametrized_value::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify parts")
    {
        edhttp::parametrized_value_list locale("en", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // make sure the result is 1 part
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 1);

        // also get the constant version
        //
        edhttp::parametrized_value::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 1);

        // now verify that the part is correct
        //
        CATCH_REQUIRE(p[0].get_name() == ""); // no name, the language is directly a value
        CATCH_REQUIRE(p[0].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        std::int64_t i(123);
        CATCH_REQUIRE_FALSE(p[0].get_parameter_as_integer("test", i));
        CATCH_REQUIRE(i == 123);
        CATCH_REQUIRE(p[0].to_string() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify weights top functions")
    {
        edhttp::parametrized_value_list locale("en-US,en;q=0.8,fr-FR;q= 0.5,fr;q =0.3", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);
//std::cerr << "--- errors? \"" << locale.error_messages() << "\"...\n";

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "en-US,en;q=0.8,fr-FR;q= 0.5,fr;q =0.3");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en-US"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"),    0.8f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr-FR"), 0.5f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"),    0.3f));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "en-US, en; q=0.8, fr-FR; q=0.5, fr; q=0.3");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify weight parts")
    {
        edhttp::parametrized_value_list locale("en-US,en;q=\"0.8\",fr-FR;q=0.5,fr;q=0.3", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // make sure the result is 4 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 4);

        // also get the constant version
        //
        edhttp::parametrized_value::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 4);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "en-US");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "en-US");

        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[1].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.8f));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "en; q=0.8");

        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[2].get_value() == "fr-FR");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.5f));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "fr-FR; q=0.5");

        CATCH_REQUIRE(p[3].get_name() == "");
        CATCH_REQUIRE(p[3].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[3].get_level(), 0.3f));
        CATCH_REQUIRE(p[3].get_parameter("test") == "");
        CATCH_REQUIRE(p[3].to_string() == "fr; q=0.3");

        CATCH_REQUIRE(pc[0].get_name() == "");
        CATCH_REQUIRE(pc[0].get_value() == "en-US");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "en-US");

        CATCH_REQUIRE(pc[1].get_name() == "");
        CATCH_REQUIRE(pc[1].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), 0.8f));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "en; q=0.8");

        CATCH_REQUIRE(pc[2].get_name() == "");
        CATCH_REQUIRE(pc[2].get_value() == "fr-FR");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.5f));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "fr-FR; q=0.5");

        CATCH_REQUIRE(pc[3].get_name() == "");
        CATCH_REQUIRE(pc[3].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[3].get_level(), 0.3f));
        CATCH_REQUIRE(pc[3].get_parameter("test") == "");
        CATCH_REQUIRE(pc[3].to_string() == "fr; q=0.3");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: sort has no effect if weights are equal")
    {
        edhttp::parametrized_value_list locale("en-US,en;q=0.8, fr-FR ; q = \"0.5\" ,fr;q=0.3", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 4);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[3].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "en-US");
        CATCH_REQUIRE(p[1].get_value() == "en");
        CATCH_REQUIRE(p[2].get_value() == "fr-FR");
        CATCH_REQUIRE(p[3].get_value() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify object")
    {
        edhttp::parametrized_value_list locale("de, en, fr", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "de, en, fr");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::parametrized_value::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "de, en, fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify part")
    {
        edhttp::parametrized_value_list locale("de, en, fr", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // also get the constant version
        //
        edhttp::parametrized_value::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "de");

        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[1].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "en");

        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[2].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "fr");

        CATCH_REQUIRE(pc[0].get_name() == "");
        CATCH_REQUIRE(pc[0].get_value() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "de");

        CATCH_REQUIRE(pc[1].get_name() == "");
        CATCH_REQUIRE(pc[1].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "en");

        CATCH_REQUIRE(pc[2].get_name() == "");
        CATCH_REQUIRE(pc[2].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: sort has no effect if weights are equal")
    {
        edhttp::parametrized_value_list locale("de, en, fr", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "de");
        CATCH_REQUIRE(p[1].get_value() == "en");
        CATCH_REQUIRE(p[2].get_value() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify object")
    {
        edhttp::parametrized_value_list locale("fr, za, en", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "fr, za, en");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::parametrized_value::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "fr, za, en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify part")
    {
        edhttp::parametrized_value_list locale("fr, za, en", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // also get the constant version
        //
        edhttp::parametrized_value::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "fr");

        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[1].get_value() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "za");

        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[2].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "en");

        CATCH_REQUIRE(pc[0].get_name() == "");
        CATCH_REQUIRE(pc[0].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "fr");

        CATCH_REQUIRE(pc[1].get_name() == "");
        CATCH_REQUIRE(pc[1].get_value() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "za");

        CATCH_REQUIRE(pc[2].get_name() == "");
        CATCH_REQUIRE(pc[2].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: sort has no effect if weights are equal")
    {
        edhttp::parametrized_value_list locale("fr, za, en", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "fr");
        CATCH_REQUIRE(p[1].get_value() == "za");
        CATCH_REQUIRE(p[2].get_value() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify object")
    {
        edhttp::parametrized_value_list locale("fr;q=0, za; q=0.6,en; q=0.4", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

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
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::parametrized_value::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "fr; q=0, za; q=0.6, en; q=0.4");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify part after copy in non-const and in const")
    {
        edhttp::parametrized_value_list locale("fr;q=0, za; q=0.6,en; q=0.4", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // also get the constant version
        //
        edhttp::parametrized_value::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[1].get_value() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "za; q=0.6");

        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[2].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.4f));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "en; q=0.4");

        CATCH_REQUIRE(pc[0].get_name() == "");
        CATCH_REQUIRE(pc[0].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), 0.0f));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(pc[1].get_name() == "");
        CATCH_REQUIRE(pc[1].get_value() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), 0.6f));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "za; q=0.6");

        CATCH_REQUIRE(pc[2].get_name() == "");
        CATCH_REQUIRE(pc[2].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.4f));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "en; q=0.4");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: sort by level (weight)")
    {
        edhttp::parametrized_value_list locale("fr;q=0, za; q=0.6,en; q=0.4", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are sorted by level
        //    "fr; q=0, za; q=0.6, en; q=0.4"
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "za");
        CATCH_REQUIRE(p[1].get_value() == "en");
        CATCH_REQUIRE(p[2].get_value() == "fr");

        // convert back to a string in the new order and with spaces
        //
        CATCH_REQUIRE(locale.to_string() == "za; q=0.6, en; q=0.4, fr; q=0");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify object")
    {
        edhttp::parametrized_value_list locale("  fr;  q=0,  za;  q=0.6,  en;  q=0.4  ", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

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

    CATCH_START_SECTION("parametrized_value_list: verify part")
    {
        edhttp::parametrized_value_list locale("  fr;  q=0,  za;  q=0.6,  en;  q=0.4  ", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // also get the constant version
        //
        edhttp::parametrized_value::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[1].get_value() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "za; q=0.6");

        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[2].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.4f));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "en; q=0.4");

        CATCH_REQUIRE(pc[0].get_name() == "");
        CATCH_REQUIRE(pc[0].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), 0.0f));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(pc[1].get_name() == "");
        CATCH_REQUIRE(pc[1].get_value() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), 0.6f));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "za; q=0.6");

        CATCH_REQUIRE(pc[2].get_name() == "");
        CATCH_REQUIRE(pc[2].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.4f));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "en; q=0.4");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: sort by level (weight)")
    {
        edhttp::parametrized_value_list locale("  fr;  q=0,  za;  q=0.6,  en;  q=0.4  ", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are sorted by level
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "za");
        CATCH_REQUIRE(p[1].get_value() == "en");
        CATCH_REQUIRE(p[2].get_value() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify object")
    {
        edhttp::parametrized_value_list locale("  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), 0.0f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), 0.4f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), 1.0f));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), -1.0f));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "fr; q=0, za, en; q=0.4, es; q=1.0");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify part")
    {
        edhttp::parametrized_value_list locale("  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 4);

        // also get the constant version
        //
        edhttp::parametrized_value::vector_t const & pc(locale.get_parts());
        CATCH_REQUIRE(pc.size() == 4);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[1].get_value() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "za");

        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[2].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.4f));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "en; q=0.4");

        CATCH_REQUIRE(p[3].get_name() == "");
        CATCH_REQUIRE(p[3].get_value() == "es");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[3].get_level(), 1.0f));
        CATCH_REQUIRE(p[3].get_parameter("test") == "");
        CATCH_REQUIRE(p[3].to_string() == "es; q=1.0");

        CATCH_REQUIRE(pc[0].get_name() == "");
        CATCH_REQUIRE(pc[0].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), 0.0f));
        CATCH_REQUIRE(pc[0].get_parameter("test") == "");
        CATCH_REQUIRE(pc[0].to_string() == "fr; q=0");

        CATCH_REQUIRE(pc[1].get_name() == "");
        CATCH_REQUIRE(pc[1].get_value() == "za");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(pc[1].get_parameter("test") == "");
        CATCH_REQUIRE(pc[1].to_string() == "za");

        CATCH_REQUIRE(pc[2].get_name() == "");
        CATCH_REQUIRE(pc[2].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.4f));
        CATCH_REQUIRE(pc[2].get_parameter("test") == "");
        CATCH_REQUIRE(pc[2].to_string() == "en; q=0.4");

        CATCH_REQUIRE(p[3].get_name() == "");
        CATCH_REQUIRE(p[3].get_value() == "es");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[3].get_level(), 1.0f));
        CATCH_REQUIRE(p[3].get_parameter("test") == "");
        CATCH_REQUIRE(p[3].to_string() == "es; q=1.0");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: sort by level (weight)")
    {
        edhttp::parametrized_value_list locale("  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        locale.sort_by_level();

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 4);

        // now verify that the parts are sorted by level
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[3].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "za");
        CATCH_REQUIRE(p[1].get_value() == "es");
        CATCH_REQUIRE(p[2].get_value() == "en");
        CATCH_REQUIRE(p[3].get_value() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify object")
    {
        edhttp::parametrized_value_list locale("de", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "de");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::parametrized_value::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "de");

        // make sure the result is 1 part
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 1);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "de");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: add \"en\"")
    {
        edhttp::parametrized_value_list locale("de", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

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
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::parametrized_value::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "de, en");

        // make sure the result is 2 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 2);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "de");

        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[1].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "en");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: add \"en\" and then \"fr\"")
    {
        edhttp::parametrized_value_list locale("de", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

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
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::parametrized_value::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "de, en, fr");

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "de");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "de");

        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[1].get_value() == "en");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(p[1].to_string() == "en");

        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[2].get_value() == "fr");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(p[2].to_string() == "fr");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: replace with \"mo\"")
    {
        edhttp::parametrized_value_list locale("de", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // the parser is expected to work (return true)
        //
        CATCH_REQUIRE(locale.parse("  mo  ", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE));

        // no error occurred
        //
        CATCH_REQUIRE(locale.error_messages().empty());

        // original string does not change
        //
        CATCH_REQUIRE(locale.get_string() == "  mo  ");

        // get_level() with correct and wrong names
        //
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("mo"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), edhttp::parametrized_value::UNDEFINED_LEVEL()));

        // convert back to a string
        //
        CATCH_REQUIRE(locale.to_string() == "mo");

        // make sure the result is 1 part
        //
        edhttp::parametrized_value::vector_t & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 1);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "mo");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(p[0].to_string() == "mo");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: set parameters with a value")
    {
        edhttp::parametrized_value_list list("de=123", edhttp::PARAMETRIZED_VALUE_MODE_STRICT_PARAMETRIZED_VALUE_LIST);
        CATCH_REQUIRE(list.error_messages().empty());
        CATCH_REQUIRE(list.get_string() == "de=123");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("mo"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("en"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("de"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("es"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(list.to_string() == "de=123");

        bool reset(list.parse("  mo  =  \"555\"  ", true));
std::cerr << "--- message: [" << list.error_messages() << "]\n";
        CATCH_REQUIRE(reset);
        CATCH_REQUIRE(list.error_messages().empty());
        CATCH_REQUIRE(list.get_string() == "  mo  =  \"555\"  ");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("mo"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("en"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("de"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("es"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(list.to_string() == "mo=555");

        CATCH_REQUIRE(list.parse("  en  =  \"  555  \"  ", true));
        CATCH_REQUIRE(list.error_messages().empty());
        CATCH_REQUIRE(list.get_string() == "  en  =  \"  555  \"  ");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("mo"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("en"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("de"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("es"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(list.to_string() == "en=\"  555  \"");

        CATCH_REQUIRE(list.parse("  es  =  555  ", true));
        CATCH_REQUIRE(list.error_messages().empty());
        CATCH_REQUIRE(list.get_string() == "  es  =  555  ");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("mo"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("en"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("de"), edhttp::parametrized_value::UNDEFINED_LEVEL()));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(list.get_level("es"), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(list.to_string() == "es=555");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify multiple parts")
    {
        edhttp::parametrized_value_list locale("fr;q=0;r=3.2;z=fancy, za; q = 0.6 ; h = \"angry\" ; object = \"color of the wand\",en; f=0.4", edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t const & p(locale.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "");
        CATCH_REQUIRE(p[0].get_value() == "fr");
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        CATCH_REQUIRE(p[0].get_parameter("r") == "3.2");
        CATCH_REQUIRE(p[0].get_parameter("z") == "fancy");
        double d(-1.0);
        CATCH_REQUIRE_FALSE(p[0].get_parameter_as_floating_point("z", d));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(d, -1.0));
        CATCH_REQUIRE_FALSE(p[0].get_parameter_as_floating_point("unknown", d));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(d, -1.0));
        CATCH_REQUIRE(p[0].to_string() == "fr; q=0; r=3.2; z=fancy");

        CATCH_REQUIRE(p[1].get_name() == "");
        CATCH_REQUIRE(p[1].get_value() == "za");
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
        CATCH_REQUIRE(p[1].get_parameter("h") == "angry");
        CATCH_REQUIRE(p[1].get_parameter("object") == "color of the wand");
        CATCH_REQUIRE(p[1].to_string() == "za; h=angry; object=\"color of the wand\"; q=0.6");

        CATCH_REQUIRE(p[2].get_name() == "");
        CATCH_REQUIRE(p[2].get_value() == "en");
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("f") == "0.4");
        CATCH_REQUIRE(p[2].to_string() == "en; f=0.4");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: verify other quotation requirements")
    {
        edhttp::parametrized_value_list strict("fr=francais;q=0;r=3.2;z=\"c'est necessaire\",,, za=\"South Africa\"; q = 0.6 ; h = \"angry\" ; object = \"color of the wand\",en; f=0.4; cute=\"girls \\\"are\\\" dancing\"", edhttp::PARAMETRIZED_VALUE_MODE_STRICT_PARAMETRIZED_VALUE_LIST);

        // make sure the result is 3 parts
        //
        edhttp::parametrized_value::vector_t & p(strict.get_parts());
        CATCH_REQUIRE(p.size() == 3);

        double d(-1.0);

        // now verify that the parts are correct
        //
        CATCH_REQUIRE(p[0].get_name() == "fr");
        CATCH_REQUIRE(p[0].get_value() == "francais");
        CATCH_REQUIRE(p[0].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
        p[0].set_level(0.5f);
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.5f));
        CATCH_REQUIRE(p[0].get_parameter("r") == "3.2");
        CATCH_REQUIRE(p[0].get_parameter_as_floating_point("r", d));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(d, 3.2));
        CATCH_REQUIRE(p[0].get_parameter("z") == "c'est necessaire");
        CATCH_REQUIRE(p[0].to_string() == "fr=francais; q=0; r=3.2; z=\"c'est necessaire\"");

        CATCH_REQUIRE(p[1].get_name() == "za");
        CATCH_REQUIRE(p[1].get_value() == "South Africa");
        CATCH_REQUIRE(p[1].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
        CATCH_REQUIRE(p[1].get_parameter_as_floating_point("q", d));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(d, 0.6));
        CATCH_REQUIRE(p[1].get_parameter("h") == "angry");
        CATCH_REQUIRE(p[1].get_parameter("object") == "color of the wand");
        CATCH_REQUIRE(p[1].to_string() == "za=\"South Africa\"; h=angry; object=\"color of the wand\"; q=0.6");

        CATCH_REQUIRE(p[2].get_name() == "en");
        CATCH_REQUIRE(p[2].get_value() == "");
        CATCH_REQUIRE(p[2].get_parameter("test") == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), edhttp::parametrized_value::DEFAULT_LEVEL()));
        CATCH_REQUIRE(p[2].get_parameter("f") == "0.4");
        CATCH_REQUIRE(p[2].get_parameter_as_floating_point("f", d));
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(d, 0.4));
        CATCH_REQUIRE(p[2].get_parameter("cute") == "girls \"are\" dancing");
        CATCH_REQUIRE(p[2].to_string() == "en; cute=\"girls \\\"are\\\" dancing\"; f=0.4");

        //std::int64_t value(-1);
        //CATCH_REQUIRE(p->has_parameter("q"));
        //CATCH_REQUIRE(p->get_parameter("q") == "joke");
        //CATCH_REQUIRE_FALSE(p->get_parameter_as_integer("q", value));
        //CATCH_REQUIRE(value == -1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: negative quality is not accepted as such (ignored)")
    {
        edhttp::parametrized_value_list locale(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        CATCH_REQUIRE(locale.parse("fr-FR;q=-1.0"));

        CATCH_REQUIRE(locale.error_messages() == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr-FR"), edhttp::parametrized_value::DEFAULT_LEVEL()));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: quality must be a valid floating point (ignored)")
    {
        edhttp::parametrized_value_list locale(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        CATCH_REQUIRE(locale.parse("fr-FR;q=joke"));

        CATCH_REQUIRE(locale.error_messages() == "");
        CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr-FR"), edhttp::parametrized_value::DEFAULT_LEVEL()));

        edhttp::parametrized_value * p(locale.get_part("fr-FR"));
        CATCH_REQUIRE(p == nullptr);

        p = locale.get_part_by_value("fr-FR");
        CATCH_REQUIRE(p != nullptr);
        std::int64_t value(-1);
        CATCH_REQUIRE(p->has_parameter("q"));
        CATCH_REQUIRE(p->get_parameter("q") == "joke");
        CATCH_REQUIRE_FALSE(p->get_parameter_as_integer("q", value));
        CATCH_REQUIRE(value == -1);

        p = locale.get_part("en-AU");
        CATCH_REQUIRE(p == nullptr);

        p = locale.get_part_by_value("en-AU");
        CATCH_REQUIRE(p == nullptr);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: retrieve an integer")
    {
        edhttp::parametrized_value_list alt_svc;

        CATCH_REQUIRE(alt_svc.parse("h3=\":443\"; ma=86400"));

        CATCH_REQUIRE(alt_svc.error_messages() == "");
        edhttp::parametrized_value * p(alt_svc.get_part("h3"));
        CATCH_REQUIRE(p != nullptr);
        CATCH_REQUIRE(p->get_name() == "h3");
        CATCH_REQUIRE(p->get_value() == ":443");
        CATCH_REQUIRE(p->get_parameter("ma") == "86400");
        std::int64_t value(-1);
        CATCH_REQUIRE(p->has_parameter("ma"));
        CATCH_REQUIRE(p->get_parameter_as_integer("ma", value));
        CATCH_REQUIRE(value == 86400);
        CATCH_REQUIRE_FALSE(p->has_parameter("persist"));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("parametrized_value_list: use a name with a %XX character")
    {
        edhttp::parametrized_value_list alt_svc;

        CATCH_REQUIRE(alt_svc.parse("%5Bh3%5D=\":443\"; ma=86400"));

        CATCH_REQUIRE(alt_svc.error_messages() == "");
        edhttp::parametrized_value * p(alt_svc.get_part("%5Bh3%5D"));
        CATCH_REQUIRE(p != nullptr);
        CATCH_REQUIRE(p->get_name() == "%5Bh3%5D");
        std::string decoded_name(edhttp::urldecode(p->get_name())); // for an alt-svc, decoding %XX is expected
        CATCH_REQUIRE(decoded_name == "[h3]");
        CATCH_REQUIRE(p->get_value() == ":443");
        CATCH_REQUIRE(p->get_parameter("ma") == "86400");
        std::int64_t value(-1);
        CATCH_REQUIRE(p->has_parameter("ma"));
        CATCH_REQUIRE(p->get_parameter_as_integer("ma", value));
        CATCH_REQUIRE(value == 86400);
        CATCH_REQUIRE_FALSE(p->has_parameter("persist"));
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("invalid_parametrized_value_list", "[invalid][string]")
{
    //CATCH_START_SECTION("invalid_parametrized_value_list: main name too long")
    //{
    //    edhttp::parametrized_value_list locale;
    //
    //    CATCH_REQUIRE_FALSE(locale.parse("deutsch_ist_zu-schwierig"));
    //
    //    CATCH_REQUIRE(locale.error_messages() == "part name is empty or too long (limit is '8-8' characters).\n");
    //}
    //CATCH_END_SECTION()

    //CATCH_START_SECTION("invalid_parametrized_value_list: sub-name too long")
    //{
    //    edhttp::parametrized_value_list locale;
    //
    //    CATCH_REQUIRE_FALSE(locale.parse("deutsch-ist_zu_schwierig"));
    //
    //    CATCH_REQUIRE(locale.error_messages() == "part sub-name is empty or too long (limit is '8-8' characters).\n");
    //}
    //CATCH_END_SECTION()

    //CATCH_START_SECTION("invalid_parametrized_value_list: too many dashes in part name")
    //{
    //    edhttp::parametrized_value_list locale;
    //
    //    CATCH_REQUIRE_FALSE(locale.parse("deutsch-ist-zu-schwierig"));
    //
    //    CATCH_REQUIRE(locale.error_messages() == "part name cannot include more than one '-'.\n");
    //}
    //CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_parametrized_value_list: spurious data")
    {
        edhttp::parametrized_value_list locale(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        CATCH_REQUIRE_FALSE(locale.parse("fr-FR;joke=\"it is\" not", true));

        CATCH_REQUIRE(locale.error_messages() == "item's parameters must be separated by a semicolon.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_parametrized_value_list: expect name to be followed by '='")
    {
        edhttp::parametrized_value_list locale(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_STRICT_PARAMETRIZED_VALUE_LIST);

        // TODO: the space should not be require to get the error?
        //       except that the `|` character is allowed in tokens
        //
        CATCH_REQUIRE_FALSE(locale.parse("fr-FR|"));
        CATCH_REQUIRE(locale.error_messages() == "the bare item name was expected to be followed by an equal sign (=) and a value.\n");

        CATCH_REQUIRE_FALSE(locale.parse("fr-FR |"));
        CATCH_REQUIRE(locale.error_messages() == "the bare item name was expected to be followed by an equal sign (=) and a value.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_parametrized_value_list: expected value separator (,) or EOS")
    {
        edhttp::parametrized_value_list locale(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_LANGUAGE);

        // TODO: the space should not be require to get the error?
        //       except that the `|` character is allowed in tokens
        //
        //CATCH_REQUIRE_FALSE(locale.parse("fr-FR|"));
        //CATCH_REQUIRE(locale.error_messages() == "item not ended properly.\n");

        CATCH_REQUIRE_FALSE(locale.parse("fr-FR\x1f"));
        CATCH_REQUIRE(locale.error_messages() == "invalid character in value.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_parametrized_value_list: missing closing quote")
    {
        edhttp::parametrized_value_list alt_svc(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_STRICT_ALT_SVC);

        CATCH_REQUIRE_FALSE(alt_svc.parse("h3=\":443"));

        CATCH_REQUIRE(alt_svc.error_messages() == "closing quote missing.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_parametrized_value_list: quoted string ends with a backslash and not closing quote")
    {
        edhttp::parametrized_value_list alt_svc(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_STRICT_ALT_SVC);

        CATCH_REQUIRE_FALSE(alt_svc.parse("h3=\":443\\"));

        CATCH_REQUIRE(alt_svc.error_messages() == "unexpected standalone backslash at the end of a quoted string.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_parametrized_value_list: '?' not allowed in bare item name")
    {
        edhttp::parametrized_value_list alt_svc(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_STRICT_ALT_SVC);

        CATCH_REQUIRE_FALSE(alt_svc.parse("h3?=\":443\""));

        CATCH_REQUIRE(alt_svc.error_messages() == "a parameter name must be a valid HTTP token.\n"
                        "unexpected token, skipping to next item.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_parametrized_value_list: quoted string ends with a backslash and not closing quote")
    {
        edhttp::parametrized_value_list alt_svc(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_STRICT_ALT_SVC);

        CATCH_REQUIRE_FALSE(alt_svc.parse("\":443\""));

        CATCH_REQUIRE(alt_svc.error_messages() == "unexpected token, skipping to next item.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_parametrized_value_list: invalid / unexpected tokens")
    {
        edhttp::parametrized_value_list alt_svc(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_STRICT_ALT_SVC);

        CATCH_REQUIRE_FALSE(alt_svc.parse("h3 = [token]"));
        CATCH_REQUIRE(alt_svc.error_messages() == "a parameter name must be a valid HTTP token.\n");

        CATCH_REQUIRE_FALSE(alt_svc.parse("h3 = ; missing = value"));
        CATCH_REQUIRE(alt_svc.error_messages() == "unexpected token (59) after the = sign of the bare item.\n");

        CATCH_REQUIRE_FALSE(alt_svc.parse("h3 = 123; color = [token]"));
        CATCH_REQUIRE(alt_svc.error_messages() == "a parameter name must be a valid HTTP token.\n"
                                                 "unexpected token after the = sign to assign a parameter's value.\n");

        CATCH_REQUIRE_FALSE(alt_svc.parse("h3 = 123;; color = blue"));
        CATCH_REQUIRE(alt_svc.error_messages() == "unexpected token in an item list of parameters.\n");

        CATCH_REQUIRE_FALSE(alt_svc.parse("h3 = 123; color += 0x550022"));
        CATCH_REQUIRE(alt_svc.error_messages() == "item parameter \"color\" must be assigned a value.\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("invalid_parametrized_value_list: quoted string with obs-text characters")
    {
        edhttp::parametrized_value_list alt_svc(std::string(), edhttp::PARAMETRIZED_VALUE_MODE_STRICT_ALT_SVC);

        for(int i(0x01); i <= 0x1F; ++i)
        {
            if(i == '\t')
            {
                continue;
            }
            std::string header("h3=\"example.com:44");
            header += static_cast<char>(i);
            header += '"';
            CATCH_REQUIRE_FALSE(alt_svc.parse(header));

            CATCH_REQUIRE(alt_svc.error_messages() == "invalid character in quoted string.\n");
        }

        {
            std::string header("h3=\"example.com:44");
            header += static_cast<char>(0x7F);
            header += '"';
            CATCH_REQUIRE_FALSE(alt_svc.parse(header));

            CATCH_REQUIRE(alt_svc.error_messages() == "invalid character in quoted string.\n");
        }

        for(int i(0x80); i <= 0xFF; ++i)
        {
            std::string header("h3=\"example.com:44");
            header += static_cast<char>(i);
            header += '"';
            CATCH_REQUIRE_FALSE(alt_svc.parse(header));

            CATCH_REQUIRE(alt_svc.error_messages() == "invalid character in quoted string.\n");
        }
    }
    CATCH_END_SECTION()
}


//CATCH_TEST_CASE("invalid_parametrized_value", "[invalid][string]")
//{
//    CATCH_START_SECTION("invalid_parametrized_value: part value includes ' and \"")
//    {
//        edhttp::parametrized_value p("invalid");
//        CATCH_REQUIRE(p.get_name() == "invalid");
//        CATCH_REQUIRE(p.get_value().empty());
//        p.set_value("c'est pas \"possible\"");
//        CATCH_REQUIRE(p.get_value() == "c'est pas \"possible\"");
//        CATCH_REQUIRE_THROWS_MATCHES(
//                  p.to_string()
//                , edhttp::unquotable_string
//                , Catch::Matchers::ExceptionMessage(
//                          "edhttp_exception: string [c'est pas \"possible\"] includes single and double quotes."));
//    }
//    CATCH_END_SECTION()
//
//    CATCH_START_SECTION("invalid_parametrized_value: part value includes \" and '")
//    {
//        edhttp::parametrized_value p("invalid");
//        CATCH_REQUIRE(p.get_name() == "invalid");
//        CATCH_REQUIRE(p.get_value().empty());
//        p.set_value("\"c'est pas possible\"");
//        CATCH_REQUIRE(p.get_value() == "\"c'est pas possible\"");
//        CATCH_REQUIRE_THROWS_MATCHES(
//                  p.to_string()
//                , edhttp::unquotable_string
//                , Catch::Matchers::ExceptionMessage(
//                          "edhttp_exception: string [\"c'est pas possible\"] includes single and double quotes."));
//    }
//    CATCH_END_SECTION()
//}



// vim: ts=4 sw=4 et
