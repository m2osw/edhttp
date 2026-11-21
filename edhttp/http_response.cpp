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
#include    "edhttp/http_response.h"

#include    "edhttp/exception.h"
#include    "edhttp/names.h"
#include    "edhttp/token.h"
//#include    "edhttp/uri.h"
//#include    "edhttp/version.h"


// snaplogger
//
//#include    <snaplogger/message.h>


// eventdispatcher
//
//#include    <eventdispatcher/exception.h>


// libaddr
//
//#include    <libaddr/addr_parser.h>


// snapdev
//
#include    <snapdev/compare_switch_string.h>
//#include    <snapdev/not_reached.h>


// C++
//
#include    <algorithm>
//#include    <iostream>
//#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



namespace
{



bool is_valid_field_value(char const * value, std::size_t length)
{
    for(std::size_t idx(0); idx < length; ++idx)
    {
        char const c(value[idx]);
        if(c != ' '
        && c != '\t'
        && (c < 0x21 || c > 0x7E))
        {
            return false;
        }
    }

    return true;
}



} // no name namespace



std::shared_ptr<http_request> http_response::get_request() const
{
    return f_request;
}


/** \brief Retrieve the entire original header.
 *
 * This function returns the original header as it was read from the source.
 * Your implementation should either call set_status_line() and add_header()
 * or make sure to call the append_original_header() function for each
 * line of raw data that your read.
 *
 * This is mainly for debug purposes since you'd have to re-parse all that
 * data to make use of it.
 *
 * \return The raw header data.
 */
std::string http_response::get_original_header() const
{
    return f_original_header;
}


http_protocol_t http_response::get_protocol() const
{
    return f_protocol;
}


http_response_code_t http_response::get_response_code() const
{
    return f_response_code;
}


/** \brief Get the HTTP reason-phrase (message in status line).
 *
 * This message is considered obsolete although if you control the
 * server you are accessing, it may be useful to check it.
 *
 * In terms of validity, it may includes a tab ('\\t') or any byte
 * from 0x20 to 0xFF except 0x7F. The bytes are likely to represent
 * ISO-8859-1 but there is no guarantees.
 *
 * The reason sent in the status line is optional. As a result this
 * function may return an empty string.
 *
 * Further, the parser is not expected to remove leading or trailing
 * spaces nor double spaces (or tabs) within the message.
 *
 * \return The HTTP reason phrase found in the status line.
 */
std::string http_response::get_reason_phrase() const
{
    return f_reason_phrase;
}


bool http_response::has_header(token const & name) const
{
    return f_header.contains(name);
}


std::string http_response::get_header(token const & name) const
{
    auto const it(f_header.find(name));
    if(it == f_header.end())
    {
        return std::string();
    }
    return it->second;
}


http_cookie const * http_response::get_cookie(std::string const & name) const
{
    auto it(std::find_if(
          f_cookies.begin()
        , f_cookies.end()
        , [&name](http_cookie const & c)
        {
            return c.get_name() == name;
        }));

    if(it == f_cookies.end())
    {
        return nullptr;
    }

    return &*it;
}


http_cookie::vector_t const & http_response::get_cookies() const
{
    return f_cookies;
}


/** \brief Return the body of the response.
 *
 * Assuming that you used the basic HTTP setup without streaming capability,
 * this is the full body from the server as a buffer of bytes.
 *
 * \note
 * If possible, you should try to use the buffer_t reference as to avoid
 * a copy of the body which could be really large.
 *
 * \todo
 * Implement versions that convert the data to as2js JSON, basic XML, etc.
 *
 * \return A buffer with the response body.
 */
buffer_t const & http_response::get_response() const
{
    return f_response;
}


/** \brief Retrieve the response as a string.
 *
 * This is similar to the get_response() function above, except that it
 * gets converted into a string. Note that means it returns a copy of
 * the data. So it is better to use the get_response() reference if you
 * can.
 *
 * \todo
 * Look at using a string_view instead.
 *
 * \return A string with the response body.
 */
std::string http_response::get_response_as_string() const
{
    return std::string(reinterpret_cast<char const *>(f_response.data()), f_response.size());
}


void http_response::set_request(std::shared_ptr<http_request> request)
{
    f_request = request;
}


/** \brief Record all the header lines.
 *
 * This function appends a raw line of header from the original input.
 * If you use the set_status_line() and add_header() then you will get
 * this automatically filled up.
 *
 * Each line is terminated by "\\r\\n".
 *
 * \param[in] header  One line of header.
 */
void http_response::append_original_header(std::string const & header)
{
    f_original_header += header;
    f_original_header += "\r\n";
}


void http_response::set_protocol(http_protocol_t protocol)
{
    f_protocol = protocol;
}


void http_response::set_response_code(http_response_code_t code)
{
    f_response_code = code;
}


void http_response::set_reason_phrase(std::string const & message)
{
    f_reason_phrase = message;
}


void http_response::add_header(token const & name, std::string const & value)
{
    if(!is_token(name))
    {
        throw invalid_token("the name of a header must be a valid token.");
    }
    if(!is_valid_field_value(value.c_str(), value.length()))
    {
        throw invalid_parameter("the value of a header cannot include control characters, except a tab, and only characters under 127 (ASCII).");
    }

    if(compare_tokens("set-cookie", name) == 0)
    {
        add_cookie(http_cookie::from_http_header(value));
    }
    else if(f_header.contains(name))
    {
        // the following well defined headers cannot be duplicated
        //
        if(compare_tokens(g_name_edhttp_field_content_length, name) == 0
        || compare_tokens(g_name_edhttp_field_content_location, name) == 0
        || compare_tokens(g_name_edhttp_field_content_range, name) == 0
        || compare_tokens(g_name_edhttp_field_content_type, name) == 0
        || compare_tokens(g_name_edhttp_field_etag, name) == 0
        || compare_tokens(g_name_edhttp_field_location, name) == 0
        || compare_tokens(g_name_edhttp_field_strict_transport_security, name) == 0
        || compare_tokens(g_name_edhttp_field_transfer_encoding, name) == 0)
        {
            throw duplicate_header("header \"" + name + "\" cannot be set more than once.");
        }

        // otherwise, we can just add to the list of values
        //
        f_header[name] += ',';
        f_header[name] += value;
    }
    else
    {
        f_header[name] = value;
    }
}


/** \brief Add a cookie to the HTTP response.
 *
 * This function adds a cookie to the HTTP response but first it makes
 * sure that the cookie is not duplicated.
 *
 * The input cookie gets duplicated in the HTTP response object. So
 * updating it later has no impact to the response itself.
 *
 * If the name of the cookie starts with "__Host-" (case insensitive),
 * then the cookie must have a path defined.
 *
 * \exception duplicate_cookie
 * The unique key used is "name + path + domain". If all three match
 * between an existing cookie and the \p cookie parameter to be added,
 * then the function raises this error.
 *
 * \param[in] cookie  The cookie to be added.
 */
void http_response::add_cookie(http_cookie const & cookie)
{
    // cookies are keyed and need to be unique based on:
    //  * name
    //  * path
    //  * domain
    //
    std::string const & name(cookie.get_name());
    std::string const & path(cookie.get_path());
    std::string const & domain(cookie.get_domain());

    if(name.length() >= 7
    && snapdev::compare_upper_starts_with_string<"__HOST-">(name))
    {
        if(!snapdev::compare_starts_with_string<"__Host-">(name))
        {
            throw invalid_parameter("found __Host-... cookie name prefix with incorrect case.");
        }

        // the "__Host-..." introducer means we must have a path,
        // no domain, and the Secure flag
        //
        if(path != "/"
        || !domain.empty()
        || !cookie.get_secure())
        {
            throw invalid_parameter("found an invalid __Host-... cookie.");
        }
    }
    else if(name.length() >= 9
         && snapdev::compare_upper_starts_with_string<"__SECURE-">(name))
    {
        if(!snapdev::compare_starts_with_string<"__Secure-">(name))
        {
            throw invalid_parameter("found __Secure-... cookie name prefix with incorrect case.");
        }

        // the "__Secure-..." introducer means we must have the Secure flag
        //
        if(!cookie.get_secure())
        {
            throw invalid_parameter("found an invalid __Secure-... cookie.");
        }
    }

    for(auto c : f_cookies)
    {
        if(c.get_name() == name
        && c.get_path() == path
        && c.get_domain() == domain)
        {
            throw duplicate_cookie("cookie already exists.");
        }
    }

    f_cookies.push_back(cookie);
}


/** \brief Set the response body.
 *
 * The HTTP request may return a body. If that's the case, then it is saved
 * using this function.
 *
 * If you receive the body in chunks, you may want to use the
 * append_response() instead. The set_response() clears any already existing
 * response.
 *
 * \param[in] response  The response to save in this http_response object.
 *
 * \sa append_response()
 */
void http_response::set_response(buffer_t const & response)
{
    f_response = response;
}


void http_response::set_response(std::string const & response)
{
    f_response.resize(response.length());
    std::copy(response.begin(), response.end(), f_response.begin());
}


/** \brief Append a chunk of the response to the response buffer.
 *
 * This function adds a chunk of data to the existing response. If you need
 * to reset the response first, do set_response(std::string()).
 *
 * \param[in] chunk  A part of the response.
 *
 * \sa set_response()
 */
void http_response::append_response(buffer_t const & chunk)
{
    f_response.insert(f_response.end(), chunk.begin(), chunk.end());
}


void http_response::append_response(std::string const & chunk)
{
    f_response.insert(f_response.end(), chunk.begin(), chunk.end());
}


/** \brief Reserve space for the response.
 *
 * If you know the size of the response (i.e. got a Content-Length) then
 * you can reserve the space ahead of time and still read the data in
 * chunk.
 */
void http_response::reserve_response(std::size_t size)
{
    f_response.reserve(size);
}


/** \brief Parse the status line of an HTTP/1.x response.
 *
 * This function checks the specified \p status_line parameter and parses
 * it as an HTTP status line.
 *
 * \code
 *     status-line   = HTTP-version SP status-code SP [ reason-phrase ]
 *
 *     HTTP-version  = HTTP-name "/" DIGIT "." DIGIT
 *     HTTP-name     = %s"HTTP"
 *
 *     status-code   = 3DIGIT
 *
 *     reason-phrase = 1*( HTAB / SP / VCHAR / obs-text )
 *
 *     DIGIT         = %x30-39      ; 0-9
 *     HTAB          = %x09         ; horizontal tab
 *     obs-text      = %x80-FF
 *     SP            = %x20         ; space
 *     VCHAR         = %x21-7E      ; visible character
 * \endcode
 *
 * In most likelihood, this is version 1.1. The function only accepts
 * versions 1.1 and 1.0.
 *
 * The function saves the protocol version which you can then get with
 * get_protocol(). The response code (the 3 digits) can be retrieved
 * with the get_response_code() function. The reason phrase, which is
 * not really trustworthy, can be retrieved with get_reason_phrase().
 * The input is also saved as one line and can be retrieved with
 * get_original_header(). That happens only if the function returns true.
 *
 * \warning
 * We do not actually accept the obs-text defined above. This is clearly
 * marked as obsolete for many reasons (i.e. the encoding would not be
 * known and it can end up being a security risk).
 *
 * \param[in] status_line  This is expected to be an HTTP status line.
 *
 * \return true if the status line was considered valid and was saved.
 */
bool http_response::parse_status_line(std::string const & status_line)
{
    // HTTP / DIGIT . DIGIT SP 3DIGIT SP is 13 characters exactly
    //
    // the SP before the optional reason phrase is mandatory
    //
    if(status_line.length() < 13)
    {
        return false;
    }

    // verify the first 13 characters
    //
    if(status_line[0] != 'H'
    || status_line[1] != 'T'
    || status_line[2] != 'T'
    || status_line[3] != 'P'
    || status_line[4] != '/'
    || status_line[5] != '1'        // we only support 1.0 and 1.1
    || status_line[6] != '.'
    || (status_line[7] != '0' && status_line[7] != '1')
    || status_line[8] != ' '
    || status_line[9] < '1' || status_line[9] > '5'
    || status_line[10] < '0' || status_line[10] > '9'
    || status_line[11] < '0' || status_line[11] > '9'
    || status_line[12] != ' ')
    {
        return false;
    }

    // the reason phrase is not valid if it includes control characters
    // other than the horizontal tab; also, I prevent any character
    // above 0x80 because it's considered obsolete; we just cannot work
    // with obsolete servers
    //
    for(std::size_t idx(13); idx < status_line.length(); ++idx)
    {
        char const c(status_line[idx]);
        if((c < ' ' && c != '\t')
        || c >= 0x7F) // we refuse obs-text characters
        {
            return false;
        }
    }

    append_original_header(status_line);

    f_protocol = status_line[7] == '0'
                    ? http_protocol_t::HTTP_PROTOCOL_1_0
                    : http_protocol_t::HTTP_PROTOCOL_1_1;

    f_response_code = (status_line[9] - '0') * 100
                    + (status_line[10] - '0') * 10
                    + (status_line[11] - '0');

    f_reason_phrase = status_line.substr(13);
    if(f_reason_phrase.empty())
    {
        char const * reason_phrase(get_reason_phrase_by_code(f_response_code));
        if(reason_phrase != nullptr)
        {
            f_reason_phrase = reason_phrase;
        }
    }

    return true;
}


/** \brief Parse the specified \p header line.
 *
 * This function parses an HTTP header line and saves the name and value
 * in the response variable member. Note that in modern HTTP we do not
 * support a multi-line header definition. It's not considered legal and
 * all servers are expected not to use that old feature.
 *
 * \code
 *     field-line     = field-name ":" OWS field-value OWS
 *     field-name     = token
 *     field-value    = *field-content
 *     field-content  = field-vchar [ 1*( SP / HTAB / field-vchar ) field-vchar ]
 *     field-vchar    = VCHAR / obs-text
 *     token          = 1*tchar
 *     tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
 *                    / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
 *                    / DIGIT / ALPHA
 *     obs-text       = %x80-FF
 *
 *     ALPHA          = %x41-5A / %x61-7A   ; A-Z / a-z
 *     DIGIT          = %x30-39             ; 0-9
 *     HTAB           = %x09                ; horizontal tab
 *     OWS            = *( SP / HTAB )
 *     SP             = %x20                ; space
 *     VCHAR          = %x21-7E             ; visible character
 * \endcode
 *
 * Once the field name and value are extracted, they are added to the
 * response using the add_header(). This may _add_ the header instead
 * (i.e. a list of Set-Cookie). It may append to the value. Or it
 * may replace the value. This is defined in the add_header() function.
 *
 * \warning
 * This function is lenient. If an exception is raised, especially for
 * a double header or cookie definition, it still just returns false
 * instead of letting the exception propagate. However, that means the
 * server sent invalid data and thus it should not be trusted. It is
 * the caller's responsibility to stop at that point.
 *
 * \param[in] header  The header to parse.
 *
 * \return false if the header cannot be parsed safely.
 *
 * \sa append_original_header()
 * \sa add_header()
 */
bool http_response::parse_header(std::string const & header)
{
    std::string::size_type pos(header.find(':'));
    if(pos == 0
    || pos == std::string::npos)
    {
        return false;
    }

    // extract and verify the field name
    //
    std::string const field_name(header.substr(0, pos));
    if(!is_token(field_name))
    {
        return false;
    }

    // skip spaces before the value
    //
    std::string::size_type end(header.length());
    for(++pos; pos < end; ++pos)
    {
        char const c(header[pos]);
        if(c != ' ' && c != '\t')
        {
            break;
        }
    }

    // skip spaces at the end
    //
    for(; end > pos; --end)
    {
        char const c(header[end - 1]);
        if(c != ' ' && c != '\t')
        {
            break;
        }
    }

    // verify that the value are only VCHAR, SP, or HTAB
    //
    if(!is_valid_field_value(header.data() + pos, end - pos))
    {
        return false;
    }

    std::string const field_value(header.substr(pos, end - pos));

    try
    {
        add_header(field_name, field_value);
        append_original_header(header);
    }
    catch(duplicate_header const & e) // some headers cannot be defined twice (i.e. "Content-Length: ...")
    {
        return false;
    }
    catch(duplicate_cookie const & e) // cookies cannot be defined twice; this is based on the name, path, and domain values
    {
        return false;
    }
    catch(invalid_parameter const & e) // cookie name start with an introducer with the wrong casing (i.e. "__SECURE-...")
    {
        return false;
    }

    return true;
}


/** \brief The reason phrase as per the specified code.
 *
 * This function returns the official reason phrase for a specific HTTP code.
 * The servers may return anything they want in the reason phrase field
 * and it may not always be 100% trustworthy. The messages returned by this
 * function, however, will not change and are exactly what a server is
 * expected to return.
 *
 * \param[in] code  The code to convert to a reason string.
 *
 * \return The reason phrase for the specified code or nullptr.
 *
 * \sa get_reason_phrase_by_code_message()
 */
char const * get_reason_phrase_by_code(http_response_code_t code)
{
    switch(code)
    {
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 102: return "Processing";
    case 103: return "Early Hints";
    //case 104: return "Upload Resumption Supported"; -- this was temporary

    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-Authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    case 207: return "Multi-Status";
    case 208: return "Already Reported";
    case 226: return "IM Used";

    case 300: return "Multiple Choices";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    case 307: return "Temporary Redirect";
    case 308: return "Permanent Redirect";

    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Content Too Large";
    case 414: return "URI Too Long";
    case 415: return "Unsupported Media Type";
    case 416: return "Range Not Satisfiable";
    case 417: return "Expectation Failed";
    //case 418: return "I'm a teapot"; -- obsolete
    case 421: return "Misdirected Request";
    case 422: return "Unprocessable Content";
    case 423: return "Locked";
    case 424: return "Failed Dependency";
    case 425: return "Too Early";
    case 426: return "Upgrade Required";
    case 428: return "Precondition Required";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 451: return "Unavailable For Legal Reasons";

    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 505: return "HTTP Version Not Supported";
    case 506: return "Variant Also Negotiates";
    case 507: return "Insufficient Storage";
    case 508: return "Loop Detected";
    //case 510: return "Not Extended "; -- obsolete
    case 511: return "Network Authentication Required";
    }

    return nullptr;
}


std::string get_reason_phrase_by_code_message(http_response_code_t code)
{
    char const * reason_phrase(get_reason_phrase_by_code(code));
    if(reason_phrase == nullptr)
    {
        return std::string("Unknown (") + std::to_string(static_cast<int>(code)) + ')';
    }

    return reason_phrase;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
