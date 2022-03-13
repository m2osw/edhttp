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


// self
//
#include    "edhttp/http_date.h"

#include    "edhttp/exception.h"
#include    "edhttp/mkgmtime.h"


// snapdev
//
#include    <snapdev/to_lower.h>
#include    <snapdev/trim_string.h>


// C++
//
#include    <iomanip>
#include    <sstream>


// C
//
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{


namespace
{


char const * g_week_day_name[] =
{
    "Sunday", "Monday", "Tuesday", "Wedneday", "Thursday", "Friday", "Saturday"
};

int const g_week_day_length[] = { 6, 6, 7, 8, 8, 6, 8 }; // strlen() of g_week_day_name's

char const * g_month_name[] =
{
    "January", "February", "Marsh", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

int const g_month_length[] = { 7, 8, 5, 5, 3, 4, 4, 6, 9, 7, 8, 8 }; // strlen() of g_month_name

int const g_month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

signed char const g_timezone_adjust[26] =
{
    /* A */ -1,
    /* B */ -2,
    /* C */ -3,
    /* D */ -4,
    /* E */ -5,
    /* F */ -6,
    /* G */ -7,
    /* H */ -8,
    /* I */ -9,
    /* J */ 0, // not used
    /* K */ -10,
    /* L */ -11,
    /* M */ -12,
    /* N */ 1,
    /* O */ 2,
    /* P */ 3,
    /* Q */ 4,
    /* R */ 5,
    /* S */ 6,
    /* T */ 7,
    /* U */ 8,
    /* V */ 9,
    /* W */ 10,
    /* X */ 11,
    /* Y */ 12,
    /* Z */ 0, // Zulu time is zero
};


}
// noname namespace

/** \brief Convert a time/date value to a string.
 *
 * This function transform a date such as the content::modified field
 * to a format that is useful to the XSL parser. It supports various
 * formats:
 *
 * \li DATE_FORMAT_SHORT -- YYYY-MM-DD
 * \li DATE_FORMAT_LONG  -- YYYY-MM-DDTHH:MM:SSZ
 * \li DATE_FORMAT_TIME  -- HH:MM:SS
 * \li DATE_FORMAT_EMAIL -- dd MMM yyyy hh:mm:ss +0000
 * \li DATE_FORMAT_HTTP  -- ddd, dd MMM yyyy hh:mm:ss +0000
 *
 * The long format includes the time.
 *
 * The HTTP format uses the day and month names in English only since
 * the HTTP protocol only expects English.
 *
 * The date is always output as UTC (opposed to local time.)
 *
 * \note
 * In order to display a date to the end user (in the HTML for the users)
 * you may want to setup the timezone information first and then use
 * the various functions supplied by the locale plugin to generate the
 * date. The locale plugin also supports formatting numbers, time,
 * messages, etc. going both ways (from the formatted data to internal
 * data and vice versa.) There is also JavaScript support for editor
 * widgets.
 *
 * \warning
 * The input value is now seconds instead of microseconds.
 *
 * \param[in] seconds  A time & date value in seconds.
 * \param[in] date_format  Which format should be used.
 *
 * \return The formatted date and time.
 */
std::string date_to_string(time_t seconds, date_format_t date_format)
{
    struct tm time_info;
    gmtime_r(&seconds, &time_info);

    char buf[256];
    buf[0] = '\0';

    switch(date_format)
    {
    case date_format_t::DATE_FORMAT_SHORT:
        strftime(buf, sizeof(buf), "%Y-%m-%d", &time_info);
        break;

    case date_format_t::DATE_FORMAT_SHORT_US:
        strftime(buf, sizeof(buf), "%m-%d-%Y", &time_info);
        break;

    case date_format_t::DATE_FORMAT_LONG:
        // TBD do we want the Z when generating time for HTML headers?
        // (it is useful for the sitemap.xml at this point)
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &time_info);
        break;

    case date_format_t::DATE_FORMAT_TIME:
        strftime(buf, sizeof(buf), "%H:%M:%S", &time_info);
        break;

    case date_format_t::DATE_FORMAT_EMAIL:
        { // dd MMM yyyy hh:mm:ss +0000
            // do it manually so the date is ALWAYS in English
            std::stringstream ss;
            ss << std::setfill('0')
               << std::setw(2) << time_info.tm_mday
               << ' '
               << g_month_name[time_info.tm_mon][0]
               << g_month_name[time_info.tm_mon][1]
               << g_month_name[time_info.tm_mon][2]
               << ' '
               << std::setw(2) << time_info.tm_year
               << ' '
               << std::setw(2) << time_info.tm_hour
               << ':'
               << std::setw(2) << time_info.tm_min
               << ':'
               << std::setw(2) << time_info.tm_sec
               << " +0000";
            return ss.str();

            //return QString("%1 %2 %3 %4:%5:%6 +0000")
            //    .arg(time_info.tm_mday, 2, 10, QChar('0'))
            //    .arg(QString::fromLatin1(g_month_name[time_info.tm_mon], 3))
            //    .arg(time_info.tm_year + 1900, 4, 10, QChar('0'))
            //    .arg(time_info.tm_hour, 2, 10, QChar('0'))
            //    .arg(time_info.tm_min, 2, 10, QChar('0'))
            //    .arg(time_info.tm_sec, 2, 10, QChar('0'));
        }
        break;

    case date_format_t::DATE_FORMAT_HTTP:
        { // ddd, dd MMM yyyy hh:mm:ss GMT
            // do it manually so the date is ALWAYS in English
            std::stringstream ss;
            ss << std::setfill('0')
               << g_week_day_name[time_info.tm_wday][0]
               << g_week_day_name[time_info.tm_wday][1]
               << g_week_day_name[time_info.tm_wday][2]
               << ", "
               << std::setw(2) << time_info.tm_mday
               << ' '
               << g_month_name[time_info.tm_mon][0]
               << g_month_name[time_info.tm_mon][1]
               << g_month_name[time_info.tm_mon][2]
               << ' '
               << std::setw(4) << time_info.tm_year + 1900
               << ' '
               << std::setw(2) << time_info.tm_hour
               << ':'
               << std::setw(2) << time_info.tm_min
               << ':'
               << std::setw(2) << time_info.tm_sec
               << " +0000";
            return ss.str();

            //return QString("%1, %2 %3 %4 %5:%6:%7 +0000")
            //    .arg(QString::fromLatin1(g_week_day_name[time_info.tm_wday], 3))
            //    .arg(time_info.tm_mday, 2, 10, QChar('0'))
            //    .arg(QString::fromLatin1(g_month_name[time_info.tm_mon], 3))
            //    .arg(time_info.tm_year + 1900, 4, 10, QChar('0'))
            //    .arg(time_info.tm_hour, 2, 10, QChar('0'))
            //    .arg(time_info.tm_min, 2, 10, QChar('0'))
            //    .arg(time_info.tm_sec, 2, 10, QChar('0'));
        }
        break;

    }

    return buf;
}


/** \brief Convert a date from a string to a time_t.
 *
 * This function transforms a date received by the client to a Unix
 * time_t value. We programmed our own because several fields are
 * optional and the strptime() function does not support such. Also
 * the strptime() uses the locale() for the day and month check
 * which is not expected for HTTP. The QDateTime object has similar
 * flaws.
 *
 * The function supports the RFC822, RFC850, and ANSI formats. On top
 * of these formats, the function understands the month name in full,
 * and the week day name and timezone parameters are viewed as optional.
 *
 * See the document we used to make sure we'd support pretty much all the
 * dates that a client might send to us:
 * http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.3.1
 *
 * Formats that we support:
 *
 * \code
 *      YYYY-MM-DD
 *      DD-MMM-YYYY HH:MM:SS TZ
 *      DD-MMM-YYYY HH:MM:SS TZ
 *      WWW, DD-MMM-YYYY HH:MM:SS TZ
 *      MMM-DD-YYYY HH:MM:SS TZ
 *      WWW MMM-DD HH:MM:SS YYYY
 * \endcode
 *
 * The month and weekday may be a 3 letter abbreviation or the full English
 * name. The month must use letters. We support the ANSI format which must
 * start with the month or week name in letters. We distinguish the ANSI
 * format from the other RFC-2616 date if it starts with a week day and is
 * followed by a space, or it directly starts with the month name.
 *
 * The year may be 2 or 4 digits.
 *
 * The timezone is optional. It may use a space or a + or a - as a separator.
 * The timezone may be an abbreviation or a 4 digit number.
 *
 * \param[in] date  The date to convert to a time_t.
 *
 * \return The date and time as a Unix time_t number, -1 if the convertion fails.
 */
time_t string_to_date(std::string const & date)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
    struct parser_t
    {
        parser_t(std::string const & date)
            : f_date(snapdev::to_lower(snapdev::trim_string(date, true, true, true)))
            , f_s(f_date.c_str())
        {
        }

        parser_t(parser_t const & rhs) = delete;
        parser_t & operator = (parser_t const & rhs) = delete;

        void skip_spaces()
        {
            while(isspace(*f_s))
            {
                ++f_s;
            }
        }

        bool parse_week_day()
        {
            // day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
            //             /  "Fri"  / "Sat" /  "Sun"
            if(f_s[0] == 'm' && f_s[1] == 'o' && f_s[2] == 'n')
            {
                f_time_info.tm_wday = 1;
            }
            else if(f_s[0] == 't' && f_s[1] == 'u' && f_s[2] == 'e')
            {
                f_time_info.tm_wday = 2;
            }
            else if(f_s[0] == 'w' && f_s[1] == 'e' && f_s[2] == 'd')
            {
                f_time_info.tm_wday = 3;
            }
            else if(f_s[0] == 't' && f_s[1] == 'h' && f_s[2] == 'u')
            {
                f_time_info.tm_wday = 4;
            }
            else if(f_s[0] == 'f' && f_s[1] == 'r' && f_s[2] == 'i')
            {
                f_time_info.tm_wday = 5;
            }
            else if(f_s[0] == 's' && f_s[1] == 'a' && f_s[2] == 't')
            {
                f_time_info.tm_wday = 6;
            }
            else if(f_s[0] == 's' && f_s[1] == 'u' && f_s[2] == 'n')
            {
                f_time_info.tm_wday = 0;
            }
            else
            {
                return false;
            }

            // check whether the other characters follow
            if(strncmp(f_s + 3, g_week_day_name[f_time_info.tm_wday] + 3, g_week_day_length[f_time_info.tm_wday] - 3) == 0)
            {
                // full day (RFC850)
                f_s += g_week_day_length[f_time_info.tm_wday];
            }
            else
            {
                f_s += 3;
            }

            return true;
        }

        bool parse_month()
        {
            // month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
            //             /  "May"  /  "Jun" /  "Jul"  /  "Aug"
            //             /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"
            if(f_s[0] == 'j' && f_s[1] == 'a' && f_s[2] == 'n')
            {
                f_time_info.tm_mon = 0;
            }
            else if(f_s[0] == 'f' && f_s[1] == 'e' && f_s[2] == 'b')
            {
                f_time_info.tm_mon = 1;
            }
            else if(f_s[0] == 'm' && f_s[1] == 'a' && f_s[2] == 'r')
            {
                f_time_info.tm_mon = 2;
            }
            else if(f_s[0] == 'a' && f_s[1] == 'p' && f_s[2] == 'r')
            {
                f_time_info.tm_mon = 3;
            }
            else if(f_s[0] == 'm' && f_s[1] == 'a' && f_s[2] == 'y')
            {
                f_time_info.tm_mon = 4;
            }
            else if(f_s[0] == 'j' && f_s[1] == 'u' && f_s[2] == 'n')
            {
                f_time_info.tm_mon = 5;
            }
            else if(f_s[0] == 'j' && f_s[1] == 'u' && f_s[2] == 'l')
            {
                f_time_info.tm_mon = 6;
            }
            else if(f_s[0] == 'a' && f_s[1] == 'u' && f_s[2] == 'g')
            {
                f_time_info.tm_mon = 7;
            }
            else if(f_s[0] == 's' && f_s[1] == 'e' && f_s[2] == 'p')
            {
                f_time_info.tm_mon = 8;
            }
            else if(f_s[0] == 'o' && f_s[1] == 'c' && f_s[2] == 't')
            {
                f_time_info.tm_mon = 9;
            }
            else if(f_s[0] == 'n' && f_s[1] == 'o' && f_s[2] == 'v')
            {
                f_time_info.tm_mon = 10;
            }
            else if(f_s[0] == 'd' && f_s[1] == 'e' && f_s[2] == 'c')
            {
                f_time_info.tm_mon = 11;
            }
            else
            {
                return false;
            }

            // check whether the other characters follow
            if(strncmp(f_s + 3, g_month_name[f_time_info.tm_mon] + 3, g_month_length[f_time_info.tm_mon] - 3) == 0)
            {
                // full month (not in the specs)
                f_s += g_month_length[f_time_info.tm_mon];
            }
            else
            {
                f_s += 3;
            }

            skip_spaces();
            return true;
        }

        bool integer(
              unsigned int min_len
            , unsigned int max_len
            , unsigned int min_value
            , unsigned int max_value
            , int & result)
        {
            unsigned int u_result = 0;
            unsigned int count(0);
            for(; *f_s >= '0' && *f_s <= '9'; ++f_s, ++count)
            {
                u_result = u_result * 10 + *f_s - '0';
            }
            if(count < min_len || count > max_len
            || u_result < min_value || u_result > max_value)
            {
                result = static_cast<int>(u_result);
                return false;
            }
            result = static_cast<int>(u_result);
            return true;
        }

        bool parse_time()
        {
            if(!integer(2, 2, 0, 23, f_time_info.tm_hour))
            {
                return false;
            }
            if(*f_s != ':')
            {
                return false;
            }
            ++f_s;
            if(!integer(2, 2, 0, 59, f_time_info.tm_min))
            {
                return false;
            }
            if(*f_s != ':')
            {
                return false;
            }
            ++f_s;
            if(!integer(2, 2, 0, 60, f_time_info.tm_sec))
            {
                return false;
            }
            skip_spaces();
            return true;
        }

        bool parse_timezone()
        {
            // any timezone?
            if(*f_s == '\0')
            {
                return true;
            }

            // XXX not too sure that the zone is properly handled at this point
            // (i.e. should I do += or -=, it may be wrong in many places...)
            //
            // The newest HTTP format is to only support "+/-####"
            //
            // zone        =  "UT"  / "GMT"
            //             /  "EST" / "EDT"
            //             /  "CST" / "CDT"
            //             /  "MST" / "MDT"
            //             /  "PST" / "PDT"
            //             /  1ALPHA
            //             / ( ("+" / "-") 4DIGIT )
            if((f_s[0] == 'u' && f_s[1] == 't' && f_s[2] == '\0')                 // UT
            || (f_s[0] == 'u' && f_s[1] == 't' && f_s[2] == 'c' && f_s[3] == '\0')  // UTC (not in the spec...)
            || (f_s[0] == 'g' && f_s[1] == 'm' && f_s[2] == 't' && f_s[3] == '\0')) // GMT
            {
                // no adjustment for UTC (GMT)
            }
            else if(f_s[0] == 'e' && f_s[1] == 's' && f_s[2] == 't' && f_s[3] == '\0') // EST
            {
                f_time_info.tm_hour -= 5;
            }
            else if(f_s[0] == 'e' && f_s[1] == 'd' && f_s[2] == 't' && f_s[3] == '\0') // EDT
            {
                f_time_info.tm_hour -= 4;
            }
            else if(f_s[0] == 'c' && f_s[1] == 's' && f_s[2] == 't' && f_s[3] == '\0') // CST
            {
                f_time_info.tm_hour -= 6;
            }
            else if(f_s[0] == 'c' && f_s[1] == 'd' && f_s[2] == 't' && f_s[3] == '\0') // CDT
            {
                f_time_info.tm_hour -= 5;
            }
            else if(f_s[0] == 'm' && f_s[1] == 's' && f_s[2] == 't' && f_s[3] == '\0') // MST
            {
                f_time_info.tm_hour -= 7;
            }
            else if(f_s[0] == 'm' && f_s[1] == 'd' && f_s[2] == 't' && f_s[3] == '\0') // MDT
            {
                f_time_info.tm_hour -= 6;
            }
            else if(f_s[0] == 'p' && f_s[1] == 's' && f_s[2] == 't' && f_s[3] == '\0') // PST
            {
                f_time_info.tm_hour -= 8;
            }
            else if(f_s[0] == 'p' && f_s[1] == 'd' && f_s[2] == 't' && f_s[3] == '\0') // PDT
            {
                f_time_info.tm_hour -= 7;
            }
            else if(f_s[0] >= 'a' && f_s[0] <= 'z' && f_s[0] != 'j' && f_s[1] == '\0')
            {
                f_time_info.tm_hour += g_timezone_adjust[f_s[0] - 'a'];
            }
            else if((f_s[0] == '+' || f_s[0] == '-')
                  && f_s[1] >= '0' && f_s[1] <= '9'
                  && f_s[2] >= '0' && f_s[2] <= '9'
                  && f_s[3] >= '0' && f_s[3] <= '9'
                  && f_s[4] >= '0' && f_s[4] <= '9'
                  && f_s[5] == '\0')
            {
                f_time_info.tm_hour += ((f_s[1] - '0') * 10 + f_s[2] - '0') * (f_s[0] == '+' ? 1 : -1);
                f_time_info.tm_min  += ((f_s[3] - '0') * 10 + f_s[4] - '0') * (f_s[0] == '+' ? 1 : -1);
            }
            else
            {
                // invalid time zone
                return false;
            }

            // WARNING: the time zone doesn't get skipped!
            return true;
        }

        bool parse_ansi()
        {
            skip_spaces();
            if(!parse_month())
            {
                return false;
            }
            if(!integer(1, 2, 1, 31, f_time_info.tm_mday))
            {
                return false;
            }
            skip_spaces();
            if(!parse_time())
            {
                return false;
            }
            if(!integer(2, 4, 0, 3000, f_time_info.tm_year))
            {
                return false;
            }
            skip_spaces();
            return parse_timezone();
        }

        bool parse_us()
        {
            skip_spaces();
            if(!parse_month())
            {
                return false;
            }
            skip_spaces();
            if(!integer(1, 2, 1, 31, f_time_info.tm_mday))
            {
                return false;
            }
            skip_spaces();
            if(!integer(2, 4, 0, 3000, f_time_info.tm_year))
            {
                return false;
            }
            skip_spaces();
            return parse_time();
        }

        bool parse()
        {
            // support for YYYY-MM-DD
            if(f_date.size() == 10
            && f_s[4] == '-'
            && f_s[7] == '-')
            {
                if(!integer(4, 4, 0, 3000, f_time_info.tm_year))
                {
                    return false;
                }
                if(*f_s != '-')
                {
                    return false;
                }
                ++f_s;
                if(!integer(2, 2, 1, 12, f_time_info.tm_mon))
                {
                    return false;
                }
                --f_time_info.tm_mon; // expect 0 to 11 in final structure
                if(*f_s != '-')
                {
                    return false;
                }
                ++f_s;
                if(!integer(2, 2, 1, 31, f_time_info.tm_mday))
                {
                    return false;
                }
                return true;
            }

            // week day (optional in RFC822)
            if(*f_s >= 'a' && *f_s <= 'z')
            {
                if(!parse_week_day())
                {
                    // maybe that was the month, not the day
                    // if the time is last, we have a preprocessor date/time
                    // the second test is needed because the string gets
                    // simplified and thus numbers 1 to 9 generate a string
                    // one shorter
                    if((strlen(f_s) == 11 + 1 + 8
                     && f_s[11 + 1 + 8 - 6] == ':'
                     && f_s[11 + 1 + 8 - 3] == ':')
                    ||
                       (strlen(f_s) == 10 + 1 + 8
                     && f_s[10 + 1 + 8 - 6] == ':'
                     && f_s[10 + 1 + 8 - 3] == ':'))
                    {
                        return parse_us();
                    }
                    return parse_ansi();
                }

                if(f_s[0] == ' ')
                {
                    // the ANSI format is completely random!
                    return parse_ansi();
                }

                if(f_s[0] != ',')
                {
                    return false;
                }
                ++f_s; // skip the comma
                skip_spaces();
            }

            if(!integer(1, 2, 1, 31, f_time_info.tm_mday))
            {
                return false;
            }

            if(*f_s == '-')
            {
                ++f_s;
            }
            skip_spaces();

            if(!parse_month())
            {
                return false;
            }
            if(*f_s == '-')
            {
                ++f_s;
                skip_spaces();
            }
            if(!integer(2, 4, 0, 3000, f_time_info.tm_year))
            {
                return false;
            }
            skip_spaces();
            if(!parse_time())
            {
                return false;
            }

            return parse_timezone();
        }

        struct tm       f_time_info = tm();
        std::string     f_date = std::string();
        char const *    f_s = nullptr;
    } parser(date);
#pragma GCC diagnostic pop

    if(!parser.parse())
    {
        return -1;
    }

    // 2 digit year?
    // How to handle this one? At this time I do not expect our software
    // to work beyond 2070 which is probably short sighted (ha! ha!)
    // However, that way we avoid calling time() and transform that in
    // a tm structure and check that date
    if(parser.f_time_info.tm_year < 100)
    {
        parser.f_time_info.tm_year += 1900;
        if(parser.f_time_info.tm_year < 1970)
        {
            parser.f_time_info.tm_year += 100;
        }
    }

    // make sure the day is valid for that month/year
    if(parser.f_time_info.tm_mday > last_day_of_month(parser.f_time_info.tm_mon + 1, parser.f_time_info.tm_year))
    {
        return -1;
    }

    // now we have a time_info which is fully adjusted except for DST...
    // let's make time
    parser.f_time_info.tm_year -= 1900;
    return mkgmtime(&parser.f_time_info);
}


/** \brief From a month and year, get the last day of the month.
 *
 * This function gives you the number of the last day of the month.
 * In all cases, except February, it returns 30 or 31.
 *
 * For the month of February, we first compute the leap year flag.
 * If the year is a leap year, then it returns 29, otherwise it
 * returns 28.
 *
 * The leap year formula is:
 *
 * \code
 *      leap = !(year % 4) && (year % 100 || !(year % 400));
 * \endcode
 *
 * \warning
 * This function throws if called with September 1752 because the
 * month has missing days within the month (days 3 to 13).
 *
 * \exception edhttp_client_server_logic_error
 * This exception is raised if the month is not between 1 and 12 inclusive
 * or if the month/year is September 1752 (because that month never existed).
 *
 * \param[in] month  A number from 1 to 12 representing a month.
 * \param[in] year  A year, including the century.
 *
 * \return Last day of month, 30, 31, or in February, 28 or 29.
 */
int last_day_of_month(int month, int year)
{
    if(month < 1 || month > 12)
    {
        throw edhttp_client_server_logic_error(
              "last_day_of_month called with "
            + std::to_string(month)
            + " as the month number");
    }

    if(month == 2)
    {
        // special case for February
        //
        // The time when people switch from Julian to Gregorian is country
        // dependent, Great Britain changed on September 2, 1752, but some
        // countries changed as late as 1952...
        //
        // For now, we use the GB date. Once we have a valid way to handle
        // this with the locale, we can look into updating the code. That
        // being said, it should not matter too much because most dates on
        // the Internet are past 2000.
        //
        if(year <= 1752)
        {
            return year % 4 == 0 ? 29 : 28;
        }
        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0) ? 29 : 28;
    }

    if(month == 9 && year == 1752)
    {
        // we cannot handle this nice one here, days 3 to 13 are missing on
        // this month... (to adjust the calendar all at once!)
        throw edhttp_client_server_logic_error(
              "last_day_of_month called with "
            + std::to_string(year)
            + " as the year number");
    }

    return g_month_days[month - 1];
}



} // namespace edhttp
// vim: ts=4 sw=4 et
