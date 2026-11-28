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

// self
//
#include    <edhttp/utils.h>



// magpie
//
#include    <magpie/cache.h>


// libutf8
//
#include    <libutf8/libutf8.h>


// snapdev
//
#include    <snapdev/compare_switch_string.h>
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/to_lower.h>


// C++
//
//#include    <string>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{
namespace
{



magpie::cache<std::string, magpie::empty_t>::pointer_t  g_accepted_protocols_cache = magpie::cache<std::string, magpie::empty_t>::pointer_t();


std::set<std::string>                                   g_accepted_domains = std::set<std::string>();



} // no name namespace



/** \brief Cache the fact that a given domain can use HTTP/2 or HTTP/3.
 *
 * This function takes a domain with a protocol set has h2: or h3:.
 * For example, we could have:
 *
 * \code
 * h2:www.example.com
 * \endcode
 *
 * This tells an HTTP/1.1 client that the server also accepts HTTP/2
 * so the next time we try to connect, we will try to use HTTP/2
 * immediately.
 *
 * The key must be the protocol, a colon, and the plain domain name.
 * So something like:
 *
 * \code
 *     <protocol>:<host>
 * \endcode
 *
 * where `<protocol>` is `h2` or `h3` and `<host>` is the host name
 * found in a http_request.
 *
 * The TTL is the ma=... value if specified in the Alt-Svc field or
 * 86400 otherwise.
 *
 * \param[in] domain  The domain with the protocol.
 * \param[in] ttl  The amount of time this value will stay around.
 *
 * \sa accepted_protocol(std::string const & domain)
 */
void accepted_protocol(std::string const & domain, magpie::duration_t ttl)
{
    if(g_accepted_protocols_cache == nullptr)
    {
        magpie::options opts;
        opts.set_default_ttl(86'400);
        opts.set_cleanup_interval(3'600);
        opts.set_count_limit(1'000);
        g_accepted_protocols_cache = std::make_shared<magpie::cache<std::string, magpie::empty_t>>(opts);
    }

    g_accepted_protocols_cache->set(domain, magpie::empty_t(), ttl);
}


bool accepted_protocol(std::string const & domain)
{
    if(g_accepted_protocols_cache == nullptr)
    {
        return false;
    }

    std::optional<magpie::empty_t> v(g_accepted_protocols_cache->get(domain));
    return v.has_value();
}


void add_accepted_domain(std::string const & domain)
{
    g_accepted_domains.insert(snapdev::to_lower(domain));
}


void remove_accepted_domain(std::string const & domain)
{
    g_accepted_domains.erase(snapdev::to_lower(domain));
}


bool is_accepted_domain(std::string const & domain)
{
    if(snapdev::compare_upper_exact_string<"LOCALHOST">(domain))
    {
        return true;
    }

    return g_accepted_domains.contains(snapdev::to_lower(domain));
}


/** \brief Many fields in the header may require decoding.
 *
 * This function decodes %XX characters from \p in and writes the
 * result in \p out.
 *
 * If any one of the escaped characters is outside of the VCHAR, SP, HTAB,
 * then the function returns false.
 *
 * The function returns false if a % character is not followed by
 * two valid hexadecimal digits. The case is insensitive. As mentioned
 * above, if the characters falls out of the VCHAR, SP and HTAB, then
 * it is considered an error and the function returns false. In other
 * words, this function does not help you decode binary data.
 *
 * \param[in] in  The input string to decode.
 * \param[out] out  The resulting string (same as \p in if no %XX).
 *
 * \return true if the decoding had no issue, false on errors.
 */
//bool decode_percents(std::string const & in, std::string & out)
//{
//    out.reserve(in.length());
//
//    for(char const * s(in.c_str()); *s != '\0'; ++s)
//    {
//        if(*s == '%')
//        {
//            if(!snapdev::is_hexdigit(s[1])
//            || !snapdev::is_hexdigit(s[2]))
//            {
//                return false;
//            }
//            char const c(snapdev::hexdigit_to_number(s[1]) * 16
//                       + snapdev::hexdigit_to_number(s[2]));
//            if((c < ' ' && c != '\t')
//            || c > '~')
//            {
//                return false;
//            }
//            out += c;
//            s += 2; // the '%' is skipped in the for() above
//        }
//        else
//        {
//            out += *s;
//        }
//    }
//
//    return true;
//}


/** \brief Check whether a string includes CTL & obs-text.
 *
 * In most cases, control characters are forbidden in HTTP fields. There is
 * one exception with the horizontal tab character ('\t') which is often
 * but not always applicable.
 *
 * Further, some field allow for UTF-8 meaning that high bytes (0x80 to
 * 0xFF) are allowed. Those need a separate check to make sure that the
 * UTF-8 codes are valid. Note that in the old days, those bytes were not
 * properly defined. They could represent other encoding such as
 * ISO-8859-1.
 *
 * \warning
 * The function returns true when it includes a control character. That's
 * when you want to generate an error:
 *
 * \code
 * if(edhttp::has_controls(s))
 * {
 *     throw ...;
 * }
 * \endcode
 *
 * \param[in] s  The string to be checked.
 * \param[in] htab_allowed  Whether '\t' is allowed.
 * \param[in] utf8_allowed  Whether bytes from 0x80 to 0xFF are allowed.
 *
 * \return true if a control was found in the string.
 */
bool has_controls(std::string const & s, bool htab_allowed, bool utf8_allowed)
{
    ssize_t utf8_start(-1);
    for(std::size_t idx(0); idx < s.size(); ++idx)
    {
        // view all values from 0x80 to 0xFF as "controls"
        //
        unsigned char const c(s[idx]);
        if(c == '\t')
        {
             if(!htab_allowed)
             {
                return true;
             }
             if(utf8_start != -1)
             {
                if(!libutf8::is_valid_utf8(s.data() + utf8_start, idx - utf8_start, false))
                {
                    return true;
                }
                utf8_start = -1;
             }
        }
        else if(c < ' ' || c == 0x7F)
        {
            return true;
        }
        else if(c >= 0x80)
        {
            if(!utf8_allowed)
            {
                return true;
            }
            if(utf8_start == -1)
            {
                utf8_start = idx;
            }
        }
    }

    return utf8_start != -1 ? !libutf8::is_valid_utf8(s.data() + utf8_start, s.length() - utf8_start, false) : false;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
