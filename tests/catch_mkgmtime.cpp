// Copyright (c) 2011-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Verify the mkgmtime() function.
 *
 * This file implements tests to verify that the mkgmtime() is able to
 * convert a struct tm back to a time_t value.
 */

// edhttp
//
#include    <edhttp/mkgmtime.h>

//#include    <edhttp/exception.h>


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
//#include    <string.h>



//extern time_t mkgmtime(struct tm * tim_p);
CATCH_TEST_CASE("mkgmtime", "[time]")
{
    CATCH_START_SECTION("mkgmtime: convert back and forth")
    {
        for(int i(0); i < 100; ++i)
        {
            // we limit the year to +/-10,000 so we limit time_t below for
            // that reason
            //
            time_t t(0);
            SNAP_CATCH2_NAMESPACE::random(t);
            t %= 253402329600;

            struct tm tim;
            gmtime_r(&t, &tim);

            // Note: at the moment, mkgmtime() is a C function
            //
            time_t const back(mkgmtime(&tim));

            CATCH_REQUIRE(t == back);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("mkgmtime: test with +/- to each segment")
    {
        time_t const t(0x67205493); // Tue Oct 29 03:20:51 AM UTC 2024

        struct tm tim;
        gmtime_r(&t, &tim);

        struct tm const org(tim);

        // as is
        //
        time_t back(mkgmtime(&tim));
        CATCH_REQUIRE(t == back);
        CATCH_REQUIRE(memcmp(&org, &tim, sizeof(org)) == 0);

        struct tm sec_only = {};
        sec_only.tm_sec = static_cast<int>(t);
        sec_only.tm_mday = 1;  // tm_mday cannot legally be 0
        sec_only.tm_year = 70; // tm_year is a number + 1900 and the time_t is since 1970, so we need to start with 70 here
        back = mkgmtime(&sec_only);
        CATCH_REQUIRE(t == back);

        // time is easy
        //
        int old_value(tim.tm_sec);
        tim.tm_sec += 93;
        back = mkgmtime(&tim);
        CATCH_REQUIRE(t + 93 == back);
        CATCH_REQUIRE(tim.tm_sec == (old_value + 93) % 60);

        tim = org;
        old_value = tim.tm_min;
        tim.tm_min += 93;
        back = mkgmtime(&tim);
        CATCH_REQUIRE(t + 93 * 60 == back);
        CATCH_REQUIRE(tim.tm_min == (old_value + 93) % 60);

        tim = org;
        old_value = tim.tm_hour;
        tim.tm_hour += 93;
        back = mkgmtime(&tim);
        CATCH_REQUIRE(t + 93 * 3600 == back);
        CATCH_REQUIRE(tim.tm_hour == (old_value + 93) % 24);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("mkgmtime_error", "[time][error]")
{
    CATCH_START_SECTION("mkgmtime: year over 10,000 do not work")
    {
        time_t const t(1098547031761);

        struct tm tim;
        gmtime_r(&t, &tim);

        // Note: at the moment, mkgmtime() is a C function
        //
        time_t const back(mkgmtime(&tim));

        CATCH_REQUIRE(back - 1);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
